#include "TemplateMatcher.h"

#include "MaaUtils/Logger.h"
#include "MaaUtils/NoWarningCV.hpp"
#include "MaaUtils/StringMisc.hpp"
#include "VisionUtils.hpp"

MAA_VISION_NS_BEGIN

TemplateMatcher::TemplateMatcher(
    cv::Mat image,
    std::vector<cv::Rect> rois,
    TemplateMatcherParam param,
    std::vector<cv::Mat> templates,
    std::string name)
    : VisionBase(std::move(image), std::move(rois), std::move(name))
    , param_(std::move(param))
    , low_score_better_(param_.method == cv::TemplateMatchModes::TM_SQDIFF || param_.method == cv::TemplateMatchModes::TM_SQDIFF_NORMED)
    , templates_(std::move(templates))
{
    analyze();
}

void TemplateMatcher::analyze()
{
    if (templates_.empty() || param_.thresholds.empty()) {
        LogError << name_ << "templates or threshold is empty" << VAR(param_.template_) << VAR(param_.thresholds);
        return;
    }

    auto start_time = std::chrono::steady_clock::now();

    for (size_t i = 0; i != templates_.size(); ++i) {
        while (next_roi()) {
            auto results = template_match(templates_.at(i));
            double threshold = i < param_.thresholds.size() ? param_.thresholds.at(i) : param_.thresholds.back();
            add_results(std::move(results), threshold);
        }
        reset_roi();
    }

    cherry_pick();

    auto cost = duration_since(start_time);
    LogDebug << name_ << VAR(all_results_) << VAR(filtered_results_) << VAR(best_result_) << VAR(cost) << VAR(param_.template_)
             << VAR(templates_.size()) << VAR(param_.thresholds) << VAR(param_.method) << VAR(param_.green_mask);
}

TemplateMatcher::ResultsVec TemplateMatcher::template_match(const cv::Mat& templ) const
{
    cv::Mat image = image_with_roi();

    if (templ.cols > image.cols || templ.rows > image.rows) {
        LogError << name_ << "templ size is too large" << VAR(image) << VAR(templ);
        return {};
    }

    bool invert_score = false;

    int method = param_.method;
    if (method >= TemplateMatcherParam::kMethodInvertBase) {
        invert_score = true;
        method -= TemplateMatcherParam::kMethodInvertBase;
    }

    cv::Mat matched;
    cv::matchTemplate(image, templ, matched, method, create_mask(templ, param_.green_mask));

    if (invert_score) {
        matched = 1.0f - matched;
    }

    ResultsVec raw_results;
    Result closest_result;
    if (low_score_better_) {
        closest_result.score = std::numeric_limits<float>::max();
    }
    for (int col = 0; col < matched.cols; ++col) {
        for (int row = 0; row < matched.rows; ++row) {
            float score = matched.at<float>(row, col);
            if (std::isnan(score) || std::isinf(score)) {
                continue;
            }

            if (comp_score(closest_result.score, score)) {
                closest_result.score = score;
                cv::Rect box(col + roi_.x, row + roi_.y, templ.cols, templ.rows);
                closest_result.box = box;
            }

            constexpr float kThreshold = 0.5f;
            if (comp_score(score, kThreshold)) {
                continue;
            }

            cv::Rect box(col + roi_.x, row + roi_.y, templ.cols, templ.rows);
            Result result { .box = box, .score = score };
            raw_results.emplace_back(result);
        }
    }
    // At least there is a result
    if (raw_results.empty()) {
        raw_results.emplace_back(closest_result);
    }

    auto nms_results = NMS(std::move(raw_results), 0.7, !low_score_better_);

    if (debug_draw_) {
        auto draw = draw_result(templ, nms_results);
        handle_draw(draw);
    }

    return nms_results;
}

cv::Mat TemplateMatcher::draw_result(const cv::Mat& templ, const ResultsVec& results) const
{
    cv::Mat image_draw = draw_roi();
    const auto color = cv::Scalar(0, 0, 255);

    for (size_t i = 0; i != results.size(); ++i) {
        const auto& res = results.at(i);
        cv::rectangle(image_draw, res.box, color, 1);

        std::string flag = std::format("{}: {:.3f}, [{}, {}, {}, {}]", i, res.score, res.box.x, res.box.y, res.box.width, res.box.height);
        cv::putText(image_draw, flag, cv::Point(res.box.x, res.box.y - 5), cv::FONT_HERSHEY_PLAIN, 1.2, color, 1);
    }

    int raw_width = image_.cols;
    cv::copyMakeBorder(image_draw, image_draw, 0, 0, 0, templ.cols, cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));
    cv::Mat draw_templ_roi = image_draw(cv::Rect(raw_width, 0, templ.cols, templ.rows));
    templ.copyTo(draw_templ_roi);

    if (!results.empty()) {
        cv::line(image_draw, cv::Point(raw_width, 0), results.front().box.tl(), color, 1);
    }

    return image_draw;
}

void TemplateMatcher::add_results(ResultsVec results, double threshold)
{
    std::ranges::copy_if(results, std::back_inserter(filtered_results_), [&](const auto& res) { return comp_score(threshold, res.score); });

    merge_vector_(all_results_, std::move(results));
}

void TemplateMatcher::cherry_pick()
{
    sort_(all_results_);
    sort_(filtered_results_);

    if (auto index_opt = pythonic_index(filtered_results_.size(), param_.result_index)) {
        best_result_ = filtered_results_.at(*index_opt);
    }
}

void TemplateMatcher::sort_(ResultsVec& results) const
{
    switch (param_.order_by) {
    case ResultOrderBy::Horizontal:
        sort_by_horizontal_(results);
        break;
    case ResultOrderBy::Vertical:
        sort_by_vertical_(results);
        break;
    case ResultOrderBy::Score:
        sort_by_score_(results, low_score_better_);
        break;
    case ResultOrderBy::Area:
        sort_by_area_(results);
        break;
    case ResultOrderBy::Random:
        sort_by_random_(results);
        break;
    default:
        LogError << "Not supported order by" << VAR(param_.order_by);
        break;
    }
}

bool TemplateMatcher::comp_score(double s1, double s2) const
{
    return low_score_better_ ? s1 > s2 : s1 < s2;
}

MAA_VISION_NS_END
