#ifdef MAA_WITH_NCNN

#include "NcnnOCRSession.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <numeric>
#include <sstream>

#include <polyclipping/clipper.hpp>

#include "MaaUtils/Logger.h"
#include "MaaUtils/NoWarningCV.hpp"
#include "NcnnSession.h"

MAA_RES_NS_BEGIN

// Everything below mirrors fastdeploy (vision/ocr/ppocr, PPOCRv4 without cls), including its quirks,
// so that the ncnn path produces the same boxes / texts as the onnxruntime path.
namespace
{

// DBDetectorPreprocessor
constexpr int kDetMaxSideLen = 960;
constexpr float kDetMean[3] = { 0.485f * 255.f, 0.456f * 255.f, 0.406f * 255.f };
constexpr float kDetNorm[3] = { 1.f / (0.229f * 255.f), 1.f / (0.224f * 255.f), 1.f / (0.225f * 255.f) };

// DBDetectorPostprocessor
constexpr double kDetDBThresh = 0.3;
constexpr float kDetDBBoxThresh = 0.6f;
constexpr float kDetDBUnclipRatio = 1.5f;
constexpr int kDetDBMaxCandidates = 1000;
constexpr int kDetMinSize = 3;

// RecognizerPreprocessor (PPOCRv3/v4 sets rec image height to 48)
constexpr int kRecImgH = 48;
constexpr int kRecImgW = 320;
constexpr float kRecMean[3] = { 127.5f, 127.5f, 127.5f };
constexpr float kRecNorm[3] = { 1.f / 127.5f, 1.f / 127.5f, 1.f / 127.5f };
constexpr double kRecPadValue = 127.;

// PPOCRv2::rec_batch_size_
constexpr size_t kRecBatchSize = 6;

using FloatBox = std::array<std::array<float, 2>, 4>;
using IntBox = std::array<std::array<int, 2>, 4>;

cv::Mat to_bgr(const cv::Mat& image)
{
    cv::Mat bgr;
    if (image.channels() == 4) {
        cv::cvtColor(image, bgr, cv::COLOR_BGRA2BGR);
    }
    else if (image.channels() == 1) {
        cv::cvtColor(image, bgr, cv::COLOR_GRAY2BGR);
    }
    else {
        bgr = image;
    }
    if (!bgr.isContinuous()) {
        bgr = bgr.clone();
    }
    return bgr;
}

// PostProcessor::GetMiniBoxes
FloatBox get_mini_boxes(const cv::RotatedRect& box, float& ssid)
{
    ssid = std::max(box.size.width, box.size.height);

    cv::Mat points;
    cv::boxPoints(box, points);

    std::vector<std::array<float, 2>> array(4);
    for (int i = 0; i < 4; ++i) {
        array[i] = { points.at<float>(i, 0), points.at<float>(i, 1) };
    }
    std::sort(array.begin(), array.end(), [](const auto& a, const auto& b) { return a[0] < b[0]; });

    std::array<float, 2> idx1 = array[0], idx2 = array[1], idx3 = array[2], idx4 = array[3];
    if (array[3][1] <= array[2][1]) {
        idx2 = array[3];
        idx3 = array[2];
    }
    else {
        idx2 = array[2];
        idx3 = array[3];
    }
    if (array[1][1] <= array[0][1]) {
        idx1 = array[1];
        idx4 = array[0];
    }
    else {
        idx1 = array[0];
        idx4 = array[1];
    }
    return { idx1, idx2, idx3, idx4 };
}

// PostProcessor::PolygonScoreAcc ("slow" score mode, the default)
float polygon_score_acc(const std::vector<cv::Point>& contour, const cv::Mat& pred)
{
    const int width = pred.cols;
    const int height = pred.rows;

    int xmin = width, xmax = 0, ymin = height, ymax = 0;
    for (const auto& p : contour) {
        xmin = std::min(xmin, p.x);
        xmax = std::max(xmax, p.x);
        ymin = std::min(ymin, p.y);
        ymax = std::max(ymax, p.y);
    }
    xmin = std::clamp(xmin, 0, width - 1);
    xmax = std::clamp(xmax, 0, width - 1);
    ymin = std::clamp(ymin, 0, height - 1);
    ymax = std::clamp(ymax, 0, height - 1);

    cv::Mat mask = cv::Mat::zeros(ymax - ymin + 1, xmax - xmin + 1, CV_8UC1);
    std::vector<cv::Point> shifted;
    shifted.reserve(contour.size());
    for (const auto& p : contour) {
        shifted.emplace_back(p.x - xmin, p.y - ymin);
    }
    const cv::Point* ppt[1] = { shifted.data() };
    const int npt[] = { static_cast<int>(shifted.size()) };
    cv::fillPoly(mask, ppt, npt, 1, cv::Scalar(1));

    cv::Mat cropped = pred(cv::Rect(xmin, ymin, xmax - xmin + 1, ymax - ymin + 1));
    return static_cast<float>(cv::mean(cropped, mask)[0]);
}

// PostProcessor::UnClip
cv::RotatedRect unclip(const FloatBox& box, float unclip_ratio)
{
    // GetContourArea
    float area = 0.f;
    float dist = 0.f;
    for (int i = 0; i < 4; ++i) {
        const auto& p = box[i];
        const auto& q = box[(i + 1) % 4];
        area += p[0] * q[1] - p[1] * q[0];
        dist += sqrtf((p[0] - q[0]) * (p[0] - q[0]) + (p[1] - q[1]) * (p[1] - q[1]));
    }
    area = fabs(float(area / 2.0));
    const float distance = area * unclip_ratio / dist;

    ClipperLib::ClipperOffset offset;
    ClipperLib::Path path;
    for (const auto& p : box) {
        path << ClipperLib::IntPoint(static_cast<int>(p[0]), static_cast<int>(p[1]));
    }
    offset.AddPath(path, ClipperLib::jtRound, ClipperLib::etClosedPolygon);

    ClipperLib::Paths solution;
    offset.Execute(solution, distance);

    std::vector<cv::Point2f> points;
    for (const auto& sub : solution) {
        for (const auto& pt : sub) {
            points.emplace_back(static_cast<float>(pt.X), static_cast<float>(pt.Y));
        }
    }
    if (points.empty()) {
        return cv::RotatedRect(cv::Point2f(0, 0), cv::Size2f(1, 1), 0);
    }
    return cv::minAreaRect(points);
}

// PostProcessor::BoxesFromBitmap
std::vector<IntBox> boxes_from_bitmap(const cv::Mat& pred, const cv::Mat& bitmap)
{
    const int width = bitmap.cols;
    const int height = bitmap.rows;

    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;
    cv::findContours(bitmap, contours, hierarchy, cv::RETR_LIST, cv::CHAIN_APPROX_SIMPLE);

    const size_t num_contours = std::min(contours.size(), static_cast<size_t>(kDetDBMaxCandidates));

    std::vector<IntBox> boxes;
    for (size_t i = 0; i < num_contours; ++i) {
        if (contours[i].size() <= 2) {
            continue;
        }
        float ssid = 0.f;
        const cv::RotatedRect box = cv::minAreaRect(contours[i]);
        const FloatBox array = get_mini_boxes(box, ssid);
        if (ssid < kDetMinSize) {
            continue;
        }

        const float score = polygon_score_acc(contours[i], pred);
        if (score < kDetDBBoxThresh) {
            continue;
        }

        const cv::RotatedRect points = unclip(array, kDetDBUnclipRatio);
        if (points.size.height < 1.001 && points.size.width < 1.001) {
            continue;
        }

        const FloatBox cliparray = get_mini_boxes(points, ssid);
        if (ssid < kDetMinSize + 2) {
            continue;
        }

        const int dest_width = pred.cols;
        const int dest_height = pred.rows;
        IntBox int_box;
        for (int p = 0; p < 4; ++p) {
            int_box[p][0] = static_cast<int>(
                std::clamp(roundf(cliparray[p][0] / float(width) * float(dest_width)), 0.f, float(dest_width)));
            int_box[p][1] = static_cast<int>(
                std::clamp(roundf(cliparray[p][1] / float(height) * float(dest_height)), 0.f, float(dest_height)));
        }
        boxes.emplace_back(int_box);
    }
    return boxes;
}

// PostProcessor::OrderPointsClockwise
IntBox order_points_clockwise(const IntBox& pts)
{
    std::vector<std::array<int, 2>> box(pts.begin(), pts.end());
    std::sort(box.begin(), box.end(), [](const auto& a, const auto& b) { return a[0] < b[0]; });

    std::array<std::array<int, 2>, 2> leftmost = { box[0], box[1] };
    std::array<std::array<int, 2>, 2> rightmost = { box[2], box[3] };
    if (leftmost[0][1] > leftmost[1][1]) {
        std::swap(leftmost[0], leftmost[1]);
    }
    if (rightmost[0][1] > rightmost[1][1]) {
        std::swap(rightmost[0], rightmost[1]);
    }
    return { leftmost[0], rightmost[0], rightmost[1], leftmost[1] };
}

// PostProcessor::FilterTagDetRes
std::vector<std::array<int, 8>> filter_tag_det_res(std::vector<IntBox> boxes, int ori_w, int ori_h, int resize_w, int resize_h)
{
    const float ratio_w = float(resize_w) / float(ori_w);
    const float ratio_h = float(resize_h) / float(ori_h);

    for (auto& box : boxes) {
        box = order_points_clockwise(box);
        for (auto& p : box) {
            // int /= float, as fastdeploy does
            p[0] = static_cast<int>(p[0] / ratio_w);
            p[1] = static_cast<int>(p[1] / ratio_h);
            p[0] = std::min(std::max(p[0], 0), ori_w - 1);
            p[1] = std::min(std::max(p[1], 0), ori_h - 1);
        }
    }

    std::vector<std::array<int, 8>> results;
    for (const auto& box : boxes) {
        const int rect_width = int(sqrt(pow(box[0][0] - box[1][0], 2) + pow(box[0][1] - box[1][1], 2)));
        const int rect_height = int(sqrt(pow(box[0][0] - box[3][0], 2) + pow(box[0][1] - box[3][1], 2)));
        if (rect_width <= 4 || rect_height <= 4) {
            continue;
        }
        results.push_back({ box[0][0], box[0][1], box[1][0], box[1][1], box[2][0], box[2][1], box[3][0], box[3][1] });
    }
    return results;
}

// ocr::SortBoxes (the swap index quirk is kept on purpose)
void sort_boxes(std::vector<std::array<int, 8>>& boxes)
{
    std::sort(boxes.begin(), boxes.end(), [](const auto& a, const auto& b) {
        if (a[1] < b[1]) {
            return true;
        }
        if (a[1] == b[1]) {
            return a[0] < b[0];
        }
        return false;
    });

    if (boxes.empty()) {
        return;
    }
    for (int i = 0; i < static_cast<int>(boxes.size()) - 1; ++i) {
        for (int j = i; j >= 0; --j) {
            if (std::abs(boxes[j + 1][1] - boxes[j][1]) < 10 && boxes[j + 1][0] < boxes[j][0]) {
                std::swap(boxes[i], boxes[i + 1]);
            }
        }
    }
}

// ocr::GetRotateCropImage
cv::Mat get_rotate_crop_image(const cv::Mat& src, const std::array<int, 8>& box)
{
    const int x_collect[4] = { box[0], box[2], box[4], box[6] };
    const int y_collect[4] = { box[1], box[3], box[5], box[7] };
    const int left = *std::min_element(x_collect, x_collect + 4);
    const int right = *std::max_element(x_collect, x_collect + 4);
    const int top = *std::min_element(y_collect, y_collect + 4);
    const int bottom = *std::max_element(y_collect, y_collect + 4);

    cv::Mat img_crop = src(cv::Rect(left, top, right - left, bottom - top));

    std::array<std::array<int, 2>, 4> points;
    for (int i = 0; i < 4; ++i) {
        points[i] = { box[2 * i] - left, box[2 * i + 1] - top };
    }

    const int crop_width = int(sqrt(pow(points[0][0] - points[1][0], 2) + pow(points[0][1] - points[1][1], 2)));
    const int crop_height = int(sqrt(pow(points[0][0] - points[3][0], 2) + pow(points[0][1] - points[3][1], 2)));

    const cv::Point2f pts_std[4] = { cv::Point2f(0.f, 0.f),
                                     cv::Point2f(static_cast<float>(crop_width), 0.f),
                                     cv::Point2f(static_cast<float>(crop_width), static_cast<float>(crop_height)),
                                     cv::Point2f(0.f, static_cast<float>(crop_height)) };
    cv::Point2f points_f[4];
    for (int i = 0; i < 4; ++i) {
        points_f[i] = cv::Point2f(static_cast<float>(points[i][0]), static_cast<float>(points[i][1]));
    }

    const cv::Mat m = cv::getPerspectiveTransform(points_f, pts_std);

    cv::Mat dst;
    // fastdeploy passes cv::BORDER_REPLICATE (== 1) as `flags`, i.e. INTER_LINEAR with constant border
    cv::warpPerspective(img_crop, dst, m, cv::Size(crop_width, crop_height), cv::INTER_LINEAR);

    if (float(dst.rows) >= float(dst.cols) * 1.5) {
        cv::Mat transposed;
        cv::transpose(dst, transposed);
        cv::flip(transposed, transposed, 0);
        return transposed;
    }
    return dst;
}

ncnn::Mat to_ncnn(const cv::Mat& bgr, const float* mean, const float* norm)
{
    ncnn::Mat in =
        ncnn::Mat::from_pixels(bgr.data, ncnn::Mat::PIXEL_BGR, bgr.cols, bgr.rows, static_cast<int>(bgr.step[0]));
    in.substract_mean_normalize(mean, norm);
    return in;
}

} // namespace

std::shared_ptr<NcnnOCRSession> NcnnOCRSession::create(const std::filesystem::path& dir, int num_threads)
{
    LogFunc << VAR(dir) << VAR(num_threads);

    auto session = std::shared_ptr<NcnnOCRSession>(new NcnnOCRSession);
    if (!session->load(dir, num_threads)) {
        return nullptr;
    }
    return session;
}

int NcnnOCRSession::det_max_side_len() const
{
    return kDetMaxSideLen;
}

NcnnOCRSession::~NcnnOCRSession()
{
    det_.reset();
    rec_.reset();
}

bool NcnnOCRSession::load(const std::filesystem::path& dir, int num_threads)
{
    // fp32 by default to stay aligned with the onnxruntime path (thresholds on det prob map, rec scores),
    // the optional det.ncnn.json / rec.ncnn.json override the ncnn options of each net.
    // default allocators: det / rec input sizes vary per call, pool allocators gain little here
    auto make_net = [&](const std::filesystem::path& sidecar) -> std::unique_ptr<ncnn::Net> {
        auto net = std::make_unique<ncnn::Net>();
        net->opt = NcnnSession::make_option(num_threads);
        if (!NcnnSession::apply_options_file(sidecar, net->opt)) {
            LogError << "invalid ncnn ocr sidecar" << VAR(sidecar);
            return nullptr;
        }
        return net;
    };
    auto has_io = [](const ncnn::Net& net) {
        return !net.input_names().empty() && !net.output_names().empty();
    };

    if (!load_keys(dir / kKeysFilename)) {
        return false;
    }

    rec_ = make_net(dir / kRecSidecarFilename);
    if (!rec_ || !NcnnSession::load_net_files(*rec_, dir / kRecParamFilename, dir / kRecBinFilename)) {
        return false;
    }
    if (!has_io(*rec_)) {
        LogError << "ncnn rec model has no input or output" << VAR(dir);
        return false;
    }

    // det is optional, but a half-shipped det is an error rather than a rec-only model
    const bool has_det_param = std::filesystem::exists(dir / kDetParamFilename);
    const bool has_det_bin = std::filesystem::exists(dir / kDetBinFilename);
    const bool has_det_sidecar = std::filesystem::exists(dir / kDetSidecarFilename);
    if (has_det_param != has_det_bin || (has_det_sidecar && !has_det_param)) {
        LogError << "ncnn det model is incomplete, expect both det.ncnn.param and det.ncnn.bin" << VAR(dir);
        return false;
    }
    if (has_det_param) {
        det_ = make_net(dir / kDetSidecarFilename);
        if (!det_ || !NcnnSession::load_net_files(*det_, dir / kDetParamFilename, dir / kDetBinFilename)) {
            return false;
        }
        if (!has_io(*det_)) {
            LogError << "ncnn det model has no input or output" << VAR(dir);
            return false;
        }
    }

    LogInfo << "ncnn ocr loaded" << VAR(dir) << VAR(labels_.size()) << VAR(rec_->opt.num_threads) << VAR(rec_->opt.use_fp16_storage)
            << VAR(det_ != nullptr);
    return true;
}

bool NcnnOCRSession::load_keys(const std::filesystem::path& path)
{
    // ReadDict: no BOM handling, "\r" trimmed, blank first and space last
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs.is_open()) {
        LogError << "failed to open keys" << VAR(path);
        return false;
    }
    std::stringstream content;
    content << ifs.rdbuf();

    labels_.clear();
    labels_.emplace_back("#");
    std::string line;
    while (std::getline(content, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        labels_.emplace_back(std::move(line));
    }
    labels_.emplace_back(" ");
    return true;
}

std::optional<std::vector<std::array<int, 8>>> NcnnOCRSession::detect(const cv::Mat& image)
{
    if (!det_) {
        LogError << "det model not loaded";
        return std::nullopt;
    }

    // OcrDetectorGetInfo
    const int w = image.cols;
    const int h = image.rows;
    float ratio = 1.f;
    if (std::max(w, h) > kDetMaxSideLen) {
        ratio = h > w ? float(kDetMaxSideLen) / float(h) : float(kDetMaxSideLen) / float(w);
    }
    int resize_h = int(float(h) * ratio);
    int resize_w = int(float(w) * ratio);
    resize_h = std::max(int(std::round(float(resize_h) / 32) * 32), 32);
    resize_w = std::max(int(std::round(float(resize_w) / 32) * 32), 32);

    cv::Mat resized;
    cv::resize(image, resized, cv::Size(resize_w, resize_h), 0, 0, cv::INTER_LINEAR);

    ncnn::Mat in = to_ncnn(resized, kDetMean, kDetNorm);
    ncnn::Mat out;
    {
        ncnn::Extractor ex = det_->create_extractor();
        if (ex.input(det_->input_names().front(), in) != 0 || ex.extract(det_->output_names().front(), out) != 0 || out.empty()) {
            LogError << "ncnn det forward failed" << VAR(image.size());
            return std::nullopt;
        }
    }
    if (!NcnnSession::is_unpacked_fp32(out)) {
        LogError << "unexpected ncnn det output layout, expect unpacked fp32" << VAR(out.elemsize) << VAR(out.elempack);
        return std::nullopt;
    }

    const int out_h = out.h;
    const int out_w = out.w;
    const float* prob = out.channel(0);
    cv::Mat pred(out_h, out_w, CV_32F, const_cast<float*>(prob));

    // DBDetectorPostprocessor::SingleBatchPostprocessor
    cv::Mat cbuf(out_h, out_w, CV_8UC1);
    for (int y = 0; y < out_h; ++y) {
        const float* src = pred.ptr<float>(y);
        auto* dst = cbuf.ptr<unsigned char>(y);
        for (int x = 0; x < out_w; ++x) {
            dst[x] = static_cast<unsigned char>(src[x] * 255);
        }
    }
    cv::Mat bitmap;
    cv::threshold(cbuf, bitmap, kDetDBThresh * 255, 255, cv::THRESH_BINARY);

    auto boxes = boxes_from_bitmap(pred, bitmap);
    return filter_tag_det_res(std::move(boxes), w, h, resize_w, resize_h);
}

std::optional<MAA_VISION_NS::OCRRecResult> NcnnOCRSession::rec_one(const cv::Mat& image, int img_w)
{
    // OcrRecognizerResizeImage (dynamic shape)
    const float ratio = float(image.cols) / float(image.rows);
    const int resize_w = ceilf(kRecImgH * ratio) > img_w ? img_w : int(ceilf(kRecImgH * ratio));

    cv::Mat resized;
    if (resize_w == image.cols && kRecImgH == image.rows) {
        resized = image;
    }
    else {
        cv::resize(image, resized, cv::Size(resize_w, kRecImgH), 0, 0, cv::INTER_LINEAR);
    }
    if (img_w > resized.cols) {
        cv::copyMakeBorder(
            resized,
            resized,
            0,
            0,
            0,
            img_w - resized.cols,
            cv::BORDER_CONSTANT,
            cv::Scalar(kRecPadValue, kRecPadValue, kRecPadValue));
    }
    if (!resized.isContinuous()) {
        resized = resized.clone();
    }

    ncnn::Mat in = to_ncnn(resized, kRecMean, kRecNorm);
    ncnn::Mat out;
    {
        ncnn::Extractor ex = rec_->create_extractor();
        if (ex.input(rec_->input_names().front(), in) != 0 || ex.extract(rec_->output_names().front(), out) != 0 || out.empty()) {
            LogError << "ncnn rec forward failed" << VAR(image.size()) << VAR(img_w);
            return std::nullopt;
        }
    }
    if (!NcnnSession::is_unpacked_fp32(out)) {
        LogError << "unexpected ncnn rec output layout, expect unpacked fp32" << VAR(out.elemsize) << VAR(out.elempack);
        return std::nullopt;
    }

    // out: [T, num_classes], softmax already applied in the model
    const int steps = out.h;
    const int num_classes = out.w;

    // RecognizerPostprocessor::SingleBatchPostprocessor
    std::string text;
    float score = 0.f;
    int count = 0;
    int last_index = 0;
    for (int n = 0; n < steps; ++n) {
        const float* row = out.row(n);
        const float* max_it = std::max_element(row, row + num_classes);
        const int argmax_idx = static_cast<int>(max_it - row);
        const float max_value = *max_it;

        if (argmax_idx > 0 && !(n > 0 && argmax_idx == last_index)) {
            score += max_value;
            count += 1;
            if (argmax_idx >= static_cast<int>(labels_.size())) {
                LogError << "rec output index exceeds keys, check keys.txt" << VAR(argmax_idx) << VAR(labels_.size())
                         << VAR(num_classes);
                return std::nullopt;
            }
            text += labels_[argmax_idx];
        }
        last_index = argmax_idx;
    }
    score /= (count + 1e-6);
    if (count == 0 || std::isnan(score)) {
        score = 0.f;
    }
    return MAA_VISION_NS::OCRRecResult { std::move(text), score };
}

std::optional<std::vector<MAA_VISION_NS::OCRRecResult>> NcnnOCRSession::rec_batch(const std::vector<const cv::Mat*>& images)
{
    // RecognizerPreprocessor::Apply: the whole batch shares the widest aspect ratio
    float max_wh_ratio = float(kRecImgW) / float(kRecImgH);
    for (const cv::Mat* image : images) {
        if (image->empty()) {
            LogError << "empty rec image";
            return std::nullopt;
        }
        max_wh_ratio = std::max(max_wh_ratio, float(image->cols) / float(image->rows));
    }
    const int img_w = int(kRecImgH * max_wh_ratio);

    std::vector<MAA_VISION_NS::OCRRecResult> results;
    results.reserve(images.size());
    for (const cv::Mat* image : images) {
        auto res = rec_one(to_bgr(*image), img_w);
        if (!res) {
            return std::nullopt;
        }
        results.emplace_back(std::move(*res));
    }
    return results;
}

std::optional<MAA_VISION_NS::OCRDetRecResult> NcnnOCRSession::det_and_rec(const cv::Mat& image_in)
{
    std::unique_lock lock(mutex_);

    if (image_in.empty()) {
        LogError << "image is empty";
        return std::nullopt;
    }
    const cv::Mat image = to_bgr(image_in);

    auto boxes_opt = detect(image);
    if (!boxes_opt) {
        return std::nullopt;
    }
    auto& boxes = *boxes_opt;
    sort_boxes(boxes);

    // PPOCRv2::BatchPredict
    std::vector<cv::Mat> image_list;
    if (boxes.empty()) {
        image_list.emplace_back(image);
    }
    else {
        image_list.reserve(boxes.size());
        for (const auto& box : boxes) {
            image_list.emplace_back(get_rotate_crop_image(image, box));
        }
    }

    std::vector<float> width_list;
    width_list.reserve(image_list.size());
    for (const auto& img : image_list) {
        width_list.push_back(float(img.cols) / img.rows);
    }
    // ocr::ArgSort
    std::vector<int> indices(image_list.size());
    std::iota(indices.begin(), indices.end(), 0);
    std::sort(indices.begin(), indices.end(), [&](int a, int b) { return width_list[a] < width_list[b]; });

    MAA_VISION_NS::OCRDetRecResult result;
    result.texts.resize(image_list.size());
    result.scores.resize(image_list.size());

    for (size_t start = 0; start < image_list.size(); start += kRecBatchSize) {
        const size_t end = std::min(start + kRecBatchSize, image_list.size());
        std::vector<const cv::Mat*> batch;
        for (size_t i = start; i < end; ++i) {
            batch.emplace_back(&image_list[indices[i]]);
        }
        auto batch_res = rec_batch(batch);
        if (!batch_res) {
            return std::nullopt;
        }
        for (size_t i = start; i < end; ++i) {
            auto& [text, score] = batch_res->at(i - start);
            result.texts[indices[i]] = std::move(text);
            result.scores[indices[i]] = score;
        }
    }

    result.boxes = std::move(boxes);
    return result;
}

std::optional<MAA_VISION_NS::OCRRecResult> NcnnOCRSession::rec(const cv::Mat& image)
{
    std::unique_lock lock(mutex_);

    auto res = rec_batch({ &image });
    if (!res || res->empty()) {
        return std::nullopt;
    }
    return std::move(res->front());
}

std::optional<std::vector<MAA_VISION_NS::OCRRecResult>> NcnnOCRSession::batch_rec(const std::vector<cv::Mat>& images)
{
    std::unique_lock lock(mutex_);

    std::vector<const cv::Mat*> batch;
    batch.reserve(images.size());
    for (const auto& image : images) {
        batch.emplace_back(&image);
    }
    return rec_batch(batch);
}

MAA_RES_NS_END

#endif // MAA_WITH_NCNN
