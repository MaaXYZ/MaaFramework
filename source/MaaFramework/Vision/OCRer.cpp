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

    ResultsVec results;

    if (ocr_result.boxes.size() != ocr_result.text.size() || ocr_result.text.size() != ocr_result.rec_scores.size()) {
        LogWarn << "Wrong ocr_result size" << VAR(ocr_result.boxes) << VAR(ocr_result.text) << VAR(ocr_result.rec_scores);

        if (ocr_result.boxes.empty() && ocr_result.text.size() == 1 && ocr_result.rec_scores.size() == 1) {
            if (auto raw_text = ocr_result.text.front(); !raw_text.empty()) {
                // 这种情况是 det 模型没出结果，整个 ROI 直接被送给了 rec 模型。凑合用吧（
                auto text = to_u16(raw_text);
                auto score = ocr_result.rec_scores.front();
                results.emplace_back(Result { .text = std::move(text), .box = { 0, 0, image_roi.cols, image_roi.rows }, .score = score });
            }
        }

        return results;
    }

    for (size_t i = 0; i != ocr_result.text.size(); ++i) {
        // the raw_box rect like ↓
        // 0 - 1
        // 3 - 2
        const auto& raw_box = ocr_result.boxes.at(i);
        int x_collect[] = { raw_box[0], raw_box[2], raw_box[4], raw_box[6] };
        int y_collect[] = { raw_box[1], raw_box[3], raw_box[5], raw_box[7] };
        auto [left, right] = std::ranges::minmax(x_collect);
        auto [top, bottom] = std::ranges::minmax(y_collect);

        auto text = to_u16(ocr_result.text.at(i));
        cv::Rect my_box(left, top, right - left, bottom - top);
        auto score = ocr_result.rec_scores.at(i);

        results.emplace_back(Result { .text = std::move(text), .box = my_box, .score = score });
    }

    return results;
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

MAA_VISION_NS_END
