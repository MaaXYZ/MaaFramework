#include "OCRer.h"

#include <ranges>
#include <shared_mutex>

#include <boost/regex.hpp>

MAA_SUPPRESS_CV_WARNINGS_BEGIN
#include "fastdeploy/vision/ocr/ppocr/dbdetector.h"
#include "fastdeploy/vision/ocr/ppocr/ppocr_v3.h"
#include "fastdeploy/vision/ocr/ppocr/recognizer.h"
MAA_SUPPRESS_CV_WARNINGS_END

#include "MaaUtils/ImageIo.h"
#include "MaaUtils/Logger.h"
#include "MaaUtils/StringMisc.hpp"
#include "VisionUtils.hpp"

MAA_VISION_NS_BEGIN

namespace
{

// 将 fastdeploy OCRResult 转换为 OCRerResult 列表
// 抽取自 predict_det_and_rec 和 batch_predict_det_rec 的共同逻辑
std::vector<OCRerResult> convert_ocr_result(const fastdeploy::vision::OCRResult& ocr_result, const cv::Size& image_size)
{
    std::vector<OCRerResult> results;

    if (ocr_result.boxes.size() != ocr_result.text.size() || ocr_result.text.size() != ocr_result.rec_scores.size()) {
        LogWarn << "Wrong ocr_result size" << VAR(ocr_result.boxes) << VAR(ocr_result.text) << VAR(ocr_result.rec_scores);

        if (ocr_result.boxes.empty() && ocr_result.text.size() == 1 && ocr_result.rec_scores.size() == 1) {
            if (auto raw_text = ocr_result.text.front(); !raw_text.empty()) {
                // det 模型没出结果，整个 ROI 直接被送给了 rec 模型
                results.emplace_back(
                    OCRerResult {
                        .text = to_u16(raw_text),
                        .box = { 0, 0, image_size.width, image_size.height },
                        .score = ocr_result.rec_scores.front(),
                    });
            }
        }
        return results;
    }

    for (size_t i = 0; i < ocr_result.text.size(); ++i) {
        // the raw_box rect like ↓
        // 0 - 1
        // 3 - 2
        const auto& raw_box = ocr_result.boxes.at(i);
        int x_collect[] = { raw_box[0], raw_box[2], raw_box[4], raw_box[6] };
        int y_collect[] = { raw_box[1], raw_box[3], raw_box[5], raw_box[7] };
        auto [left, right] = std::ranges::minmax(x_collect);
        auto [top, bottom] = std::ranges::minmax(y_collect);

        results.emplace_back(
            OCRerResult {
                .text = to_u16(ocr_result.text.at(i)),
                .box = cv::Rect(left, top, right - left, bottom - top),
                .score = ocr_result.rec_scores.at(i),
            });
    }
    return results;
}

} // namespace

const boost::wregex& OCRer::gen_regex(const std::wstring& pattern)
{
    static std::shared_mutex mtx;
    static std::unordered_map<std::wstring, boost::wregex> s_cache;

    {
        std::shared_lock slock(mtx);
        if (auto it = s_cache.find(pattern); it != s_cache.end()) {
            return it->second;
        }
    }

    std::unique_lock ulock(mtx);
    // PipelineParser 里已经检查过正则的合法性了
    return s_cache.emplace(pattern, boost::wregex(pattern)).first->second;
}

OCRer::OCRer(
    cv::Mat image,
    std::vector<cv::Rect> rois,
    OCRerParam param,
    std::shared_ptr<fastdeploy::vision::ocr::DBDetector> deter,
    std::shared_ptr<fastdeploy::vision::ocr::Recognizer> recer,
    std::shared_ptr<fastdeploy::pipeline::PPOCRv3> ocrer,
    std::string name)
    : VisionBase(std::move(image), std::move(rois), std::move(name))
    , param_(std::move(param))
    , deter_(std::move(deter))
    , recer_(std::move(recer))
    , ocrer_(std::move(ocrer))
{
    analyze();
}

OCRer::OCRer(cv::Mat image, std::vector<cv::Rect> rois, OCRerParam param, const OCRBatchCacheValue& cached_results, std::string name)
    : VisionBase(std::move(image), std::move(rois), std::move(name))
    , param_(std::move(param))
{
    analyze_cached(cached_results);
}

void OCRer::analyze()
{
    auto start_time = std::chrono::steady_clock::now();

    while (next_roi()) {
        auto results = predict();
        add_results(std::move(results), param_.expected);
    }

    cherry_pick();

    auto cost = duration_since(start_time);
    LogDebug << name_ << VAR(all_results_) << VAR(filtered_results_) << VAR(best_result_) << VAR(cost) << VAR(param_.model)
             << VAR(param_.only_rec) << VAR(param_.expected);
}

OCRer::ResultsVec OCRer::predict() const
{
    ResultsVec results;

    auto image_roi = image_with_roi();
    results = param_.only_rec ? ResultsVec { predict_only_rec(image_roi) } : predict_det_and_rec(image_roi);

    std::ranges::for_each(results, [&](auto& res) {
        res.box.x += roi_.x;
        res.box.y += roi_.y;
    });

    if (debug_draw_) {
        auto draw = draw_result(results);
        handle_draw(draw);
    }

    return results;
}

OCRer::ResultsVec OCRer::predict_det_and_rec(const cv::Mat& image_roi) const
{
    if (!ocrer_) {
        LogError << "ocrer_ is null";
        return {};
    }

    fastdeploy::vision::OCRResult ocr_result;

    bool ret = false;
    {
        std::unique_lock lock(s_predict_mutex_);
        ret = ocrer_->Predict(image_roi, &ocr_result);
    }
    if (!ret) {
        LogWarn << "predict return false" << VAR(ocrer_) << VAR(image_) << VAR(image_roi);
        return {};
    }

    return convert_ocr_result(ocr_result, image_roi.size());
}

OCRer::Result OCRer::predict_only_rec(const cv::Mat& image_roi) const
{
    if (!recer_) {
        LogError << "recer_ is null";
        return {};
    }

    std::string reco_text;
    float reco_score = 0;
    bool ret = false;
    {
        std::unique_lock lock(s_predict_mutex_);
        ret = recer_->Predict(image_roi, &reco_text, &reco_score);
    }
    if (!ret) {
        LogWarn << "recer_ return false" << VAR(recer_) << VAR(image_) << VAR(image_roi);
        return {};
    }

    auto text = to_u16(reco_text);
    Result result { .text = std::move(text), .box = { 0, 0, image_roi.cols, image_roi.rows }, .score = reco_score };

    return result;
}

cv::Mat OCRer::draw_result(const ResultsVec& results) const
{
    cv::Mat image_draw = draw_roi();

    for (size_t i = 0; i != results.size(); ++i) {
        const cv::Rect& my_box = results.at(i).box;

        const auto color = cv::Scalar(0, 0, 255);
        cv::rectangle(image_draw, my_box, color, 1);
        std::string flag = std::format("{}: [{}, {}, {}, {}]", i, my_box.x, my_box.y, my_box.width, my_box.height);
        cv::putText(image_draw, flag, cv::Point(my_box.x, my_box.y - 5), cv::FONT_HERSHEY_PLAIN, 1.2, color, 1);
    }

    return image_draw;
}

void OCRer::add_results(ResultsVec results, const std::vector<std::wstring>& expected)
{
    auto copied = results;
    for (auto& res : copied) {
        if (res.score < param_.threshold) {
            continue;
        }

        postproc_trim_(res);
        postproc_replace_(res);

        if (!filter_by_required(res, expected)) {
            continue;
        }

        filtered_results_.emplace_back(std::move(res));
    }

    merge_vector_(all_results_, std::move(results));
}

void OCRer::cherry_pick()
{
    sort_(all_results_);
    sort_(filtered_results_);

    if (auto index_opt = pythonic_index(filtered_results_.size(), param_.result_index)) {
        best_result_ = filtered_results_.at(*index_opt);
    }
}

void OCRer::postproc_trim_(Result& res) const
{
    string_trim_(res.text);
}

void OCRer::postproc_replace_(Result& res) const
{
    for (const auto& [regex, format] : param_.replace) {
        auto replaced_text = boost::regex_replace(res.text, gen_regex(regex), format);
        LogDebug << VAR(res.text) << VAR(regex) << VAR(format) << VAR(replaced_text);
        res.text = std::move(replaced_text);
    }
}

bool OCRer::filter_by_required(const Result& res, const std::vector<std::wstring>& expected) const
{
    if (expected.empty()) {
        return true;
    }

    for (const auto& regex : expected) {
        if (boost::regex_search(res.text, gen_regex(regex))) {
            return true;
        }
    }

    return false;
}

void OCRer::sort_(ResultsVec& results) const
{
    switch (param_.order_by) {
    case ResultOrderBy::Horizontal:
        sort_by_horizontal_(results);
        break;
    case ResultOrderBy::Vertical:
        sort_by_vertical_(results);
        break;
    // case ResultOrderBy::Score:
    //     sort_by_score_(results);
    //     break;
    case ResultOrderBy::Area:
        sort_by_area_(results);
        break;
    case ResultOrderBy::Random:
        sort_by_random_(results);
        break;
    case ResultOrderBy::Length:
        std::ranges::sort(results, [](const auto& lhs, const auto& rhs) -> bool { return lhs.text.size() > rhs.text.size(); });
        break;
    case ResultOrderBy::Expected:
        sort_by_expected_regex_(results, param_.expected);
        break;

    default:
        LogError << "Not supported order by" << VAR(param_.order_by);
        break;
    }
}

void OCRer::analyze_cached(const OCRBatchCacheValue& cached)
{
    auto start_time = std::chrono::steady_clock::now();

    size_t total_rois = 0;
    // count how many ROIs this OCRer instance has (peek without consuming)
    // We'll count via used_cached + remaining after loop

    size_t used_cached = 0;
    for (size_t i = 0; i < cached.per_roi_results.size() && next_roi(); ++i) {
        auto results = cached.per_roi_results[i];
        ++used_cached;

        LogDebug << name_ << "(cached) ROI" << VAR(i) << VAR(roi_) << VAR(results.size());

        if (debug_draw_) {
            auto draw = draw_result(results);
            handle_draw(draw);
        }

        add_results(std::move(results), param_.expected);
    }

    if (used_cached < cached.per_roi_results.size()) {
        LogWarn << name_ << "(cached) unused cached ROI results" << VAR(used_cached) << VAR(cached.per_roi_results.size());
    }
    if (used_cached > 0 && next_roi()) {
        LogWarn << name_ << "(cached) more ROIs than cached results, some ROIs will have no results" << VAR(used_cached)
                << VAR(cached.per_roi_results.size());
    }

    cherry_pick();

    auto cost = duration_since(start_time);
    LogDebug << name_ << "(cached)" << VAR(all_results_) << VAR(filtered_results_) << VAR(best_result_) << VAR(cost);
}

std::vector<OCRerResult>
    OCRer::batch_predict_only_rec(const std::vector<cv::Mat>& images, const std::shared_ptr<fastdeploy::vision::ocr::Recognizer>& recer)
{
    if (!recer || images.empty()) {
        return {};
    }

    std::vector<std::string> texts;
    std::vector<float> scores;
    bool ret = false;
    {
        std::unique_lock lock(s_predict_mutex_);
        ret = recer->BatchPredict(images, &texts, &scores);
    }
    if (!ret) {
        LogWarn << "BatchPredict only_rec failed";
        return {};
    }

    if (texts.size() != scores.size()) {
        LogWarn << "Mismatched batch only_rec text/score size" << VAR(texts.size()) << VAR(scores.size());
    }

    std::vector<OCRerResult> results;
    auto count = std::min({ texts.size(), scores.size(), images.size() });
    results.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        results.emplace_back(
            OCRerResult {
                .text = to_u16(texts[i]),
                .box = { 0, 0, images[i].cols, images[i].rows },
                .score = scores[i],
            });
    }
    return results;
}

std::vector<std::vector<OCRerResult>>
    OCRer::batch_predict_det_rec(const std::vector<cv::Mat>& images, const std::shared_ptr<fastdeploy::pipeline::PPOCRv3>& ocrer)
{
    if (!ocrer || images.empty()) {
        return {};
    }

    std::vector<fastdeploy::vision::OCRResult> batch_results;
    bool ret = false;
    {
        std::unique_lock lock(s_predict_mutex_);
        ret = ocrer->BatchPredict(images, &batch_results);
    }
    if (!ret) {
        LogWarn << "BatchPredict det_rec failed";
        return {};
    }

    std::vector<std::vector<OCRerResult>> all_results;
    all_results.reserve(batch_results.size());

    for (size_t img_idx = 0; img_idx < batch_results.size(); ++img_idx) {
        all_results.emplace_back(convert_ocr_result(batch_results[img_idx], images[img_idx].size()));
    }

    return all_results;
}

MAA_VISION_NS_END
