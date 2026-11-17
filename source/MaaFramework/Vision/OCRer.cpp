#include "OCRer.h"

#include <ranges>
#include <shared_mutex>

#include <boost/regex.hpp>

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
    OCRSessionPtr session,
    std::string name,
    std::optional<ColorFilterConfig> color_filter,
    OCRCache* cache)
    : VisionBase(std::move(image), std::move(rois), std::move(name))
    , param_(std::move(param))
    , color_filter_(std::move(color_filter))
    , cache_(cache)
    , session_(std::move(session))
{
    analyze();
}

void OCRer::analyze()
{
    auto start_time = std::chrono::steady_clock::now();

    // Enlarging a crop changes detector resizing and context, even below its size limit.
    // Share only exact crops; cache raw results before each node's filtering and sorting.
    const bool use_cache = cache_ && session_ && session_->supports_batch_cache() && !param_.only_rec && !color_filter_;
    while (next_roi()) {
        ResultsVec results;
        const auto* cached = use_cache ? cache_->find(session_, roi_) : nullptr;
        if (cached) {
            LogDebug << "OCR using inference cache" << VAR(name_) << VAR(roi_);
            results = *cached;
        }
        else {
            auto predicted = predict();
            if (!predicted) {
                continue;
            }
            results = std::move(*predicted);
            if (use_cache) {
                cache_->insert(session_, roi_, results);
            }
        }

        if (debug_draw_) {
            auto draw = draw_result(results);
            handle_draw(draw);
        }

        add_results(std::move(results), param_.expected);
    }

    cherry_pick();

    auto cost = duration_since(start_time);
    LogDebug << name_ << VAR(all_results_) << VAR(filtered_results_) << VAR(best_result_) << VAR(cost) << VAR(param_.model)
             << VAR(param_.only_rec) << VAR(param_.expected);
}

cv::Mat OCRer::apply_color_filter(const cv::Mat& image_roi) const
{
    const auto& cfg = *color_filter_;

    cv::Mat color;
    cv::cvtColor(image_roi, color, cfg.method);

    cv::Mat bin = cv::Mat::zeros(image_roi.size(), CV_8UC1);
    for (const auto& [lower, upper] : cfg.range) {
        cv::Mat single;
        cv::inRange(color, lower, upper, single);
        cv::bitwise_or(bin, single, bin);
    }

    cv::Mat result;
    cv::cvtColor(bin, result, cv::COLOR_GRAY2BGR);
    return result;
}

std::optional<OCRer::ResultsVec> OCRer::predict() const
{
    if (param_.only_rec && session_ && !session_->supports_rec_only()) {
        LogError << "OCR engine does not support only_rec" << VAR(param_.engine);
        return { };
    }
    ResultsVec results;

    auto image_roi = image_with_roi();
    if (color_filter_) {
        image_roi = apply_color_filter(image_roi);
    }
    if (param_.only_rec) {
        auto result = predict_only_rec(image_roi);
        if (!result) {
            return std::nullopt;
        }
        results.emplace_back(std::move(*result));
    }
    else {
        auto predicted = predict_det_and_rec(image_roi);
        if (!predicted) {
            return std::nullopt;
        }
        results = std::move(*predicted);
    }

    std::ranges::for_each(results, [&](auto& res) {
        res.box.x += roi_.x;
        res.box.y += roi_.y;
    });

    return results;
}

std::optional<OCRer::ResultsVec> OCRer::predict_det_and_rec(const cv::Mat& image_roi) const
{
    if (!session_) {
        LogError << "OCRSession not loaded";
        return { };
    }

    std::optional<OCRDetRecResult> ocr_opt;
    {
        std::unique_lock lock(s_predict_mutex_);
        ocr_opt = session_->det_and_rec(image_roi);
    }
    if (!ocr_opt) {
        LogWarn << "det_and_rec failed" << VAR(image_) << VAR(image_roi);
        return { };
    }
    const auto& [boxes, texts, scores] = *ocr_opt;

    ResultsVec results;

    if (boxes.size() != texts.size() || texts.size() != scores.size()) {
        LogWarn << "Wrong ocr_result size" << VAR(boxes) << VAR(texts) << VAR(scores);

        if (boxes.empty() && texts.size() == 1 && scores.size() == 1) {
            if (const auto& raw_text = texts.front(); !raw_text.empty()) {
                // 这种情况是 det 模型没出结果，整个 ROI 直接被送给了 rec 模型。凑合用吧（
                results.emplace_back(
                    Result { .text = to_u16(raw_text), .box = { 0, 0, image_roi.cols, image_roi.rows }, .score = scores.front() });
            }
            return results;
        }

        return std::nullopt;
    }

    for (size_t i = 0; i != texts.size(); ++i) {
        // the raw_box rect like ↓
        // 0 - 1
        // 3 - 2
        const auto& raw_box = boxes.at(i);
        int x_collect[] = { raw_box[0], raw_box[2], raw_box[4], raw_box[6] };
        int y_collect[] = { raw_box[1], raw_box[3], raw_box[5], raw_box[7] };
        auto [left, right] = std::ranges::minmax(x_collect);
        auto [top, bottom] = std::ranges::minmax(y_collect);

        cv::Rect my_box(left, top, right - left, bottom - top);
        results.emplace_back(Result { .text = to_u16(texts.at(i)), .box = my_box, .score = scores.at(i) });
    }

    return results;
}

std::optional<OCRer::Result> OCRer::predict_only_rec(const cv::Mat& image_roi) const
{
    if (session_ && !session_->supports_rec_only()) {
        LogError << "OCR engine does not support only_rec" << VAR(param_.engine);
        return { };
    }
    if (!session_) {
        LogError << "OCRSession not loaded";
        return { };
    }

    std::optional<OCRRecResult> rec_opt;
    {
        std::unique_lock lock(s_predict_mutex_);
        rec_opt = session_->rec(image_roi);
    }
    if (!rec_opt) {
        LogWarn << "rec failed" << VAR(image_) << VAR(image_roi);
        return { };
    }
    auto& [reco_text, reco_score] = *rec_opt;

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

    if (color_filter_) {
        auto bin = apply_color_filter(image_);

        int raw_width = image_draw.cols;
        cv::copyMakeBorder(image_draw, image_draw, 0, 0, 0, bin.cols, cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));
        bin.copyTo(image_draw(cv::Rect(raw_width, 0, bin.cols, bin.rows)));
    }

    return image_draw;
}

void OCRer::add_results(ResultsVec results, const std::vector<std::wstring>& expected)
{
    auto copied = results;
    for (auto& res : copied) {
        // An unknown confidence can only pass when score filtering is disabled.
        if (res.score < 0 ? param_.threshold > 0 : res.score < param_.threshold) {
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
