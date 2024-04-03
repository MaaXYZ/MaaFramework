#include "TemplateMatcher.h"

#include "Utils/Logger.h"
#include "Utils/NoWarningCV.hpp"
#include "Utils/StringMisc.hpp"
#include "VisionUtils.hpp"

MAA_VISION_NS_BEGIN

TemplateMatcher::TemplateMatcher(
    cv::Mat image,
    TemplateMatcherParam param,
    std::vector<std::shared_ptr<cv::Mat>> templates,
    std::string name)
    : VisionBase(std::move(image), std::move(name))
    , param_(std::move(param))
    , templates_(std::move(templates))
{
    analyze();
}

void TemplateMatcher::analyze()
{
    if (templates_.empty()) {
        LogError << name_ << VAR(uid_) << "templates is empty" << VAR(param_.template_paths);
        return;
    }

    if (templates_.size() != param_.thresholds.size()) {
        LogError << name_ << VAR(uid_) << "templates.size() != thresholds.size()"
                 << VAR(templates_.size()) << VAR(param_.thresholds.size());
        return;
    }

    auto start_time = std::chrono::steady_clock::now();

    for (size_t i = 0; i != templates_.size(); ++i) {
        const auto& templ = templates_.at(i);
        if (!templ) {
            continue;
        }

        auto results = match_all_rois(*templ);
        add_results(std::move(results), param_.thresholds.at(i));
    }

    cherry_pick();

    auto cost = duration_since(start_time);
    LogTrace << name_ << VAR(uid_) << VAR(all_results_) << VAR(filtered_results_)
             << VAR(best_result_) << VAR(cost);
}

TemplateMatcher::ResultsVec TemplateMatcher::match_all_rois(const cv::Mat& templ) const
{
    if (templ.empty()) {
        LogWarn << name_ << VAR(uid_) << "template is empty" << VAR(param_.template_paths)
                << VAR(templ.size());
        return {};
    }

    if (param_.roi.empty()) {
        return template_match(cv::Rect(0, 0, image_.cols, image_.rows), templ);
    }
    else {
        ResultsVec results;
        for (const cv::Rect& roi : param_.roi) {
            auto res = template_match(roi, templ);
            merge_vector_(results, std::move(res));
        }
        return results;
    }
}

TemplateMatcher::ResultsVec
    TemplateMatcher::template_match(const cv::Rect& roi, const cv::Mat& templ) const
{
    cv::Mat image = image_with_roi(roi);

    if (templ.cols > image.cols || templ.rows > image.rows) {
        LogError << name_ << VAR(uid_) << "templ size is too large" << VAR(image) << VAR(templ);
        return {};
    }

    cv::Mat matched;
    cv::matchTemplate(image, templ, matched, param_.method, create_mask(templ, param_.green_mask));

    ResultsVec raw_results;
    Result max_result;
    for (int col = 0; col < matched.cols; ++col) {
        for (int row = 0; row < matched.rows; ++row) {
            float score = matched.at<float>(row, col);
            if (std::isnan(score) || std::isinf(score)) {
                continue;
            }

            if (max_result.score < score) {
                max_result.score = score;
                cv::Rect box(col + roi.x, row + roi.y, templ.cols, templ.rows);
                max_result.box = box;
            }

            constexpr float kThreshold = 0.5f;
            if (score < kThreshold) {
                continue;
            }
            cv::Rect box(col + roi.x, row + roi.y, templ.cols, templ.rows);
            Result result { .box = box, .score = score };
            raw_results.emplace_back(result);
        }
    }
    // At least there is a result
    if (raw_results.empty()) {
        raw_results.emplace_back(max_result);
    }

    auto nms_results = NMS(std::move(raw_results));

    if (debug_draw_) {
        auto draw = draw_result(roi, templ, nms_results);
        handle_draw(draw);
    }

    return nms_results;
}

cv::Mat TemplateMatcher::draw_result(
    const cv::Rect& roi,
    const cv::Mat& templ,
    const ResultsVec& results) const
{
    cv::Mat image_draw = draw_roi(roi);
    const auto color = cv::Scalar(0, 0, 255);

    for (size_t i = 0; i != results.size(); ++i) {
        const auto& res = results.at(i);
        cv::rectangle(image_draw, res.box, color, 1);

        std::string flag = std::format(
            "{}: {:.3f}, [{}, {}, {}, {}]",
            i,
            res.score,
            res.box.x,
            res.box.y,
            res.box.width,
            res.box.height);
        cv::putText(
            image_draw,
            flag,
            cv::Point(res.box.x, res.box.y - 5),
            cv::FONT_HERSHEY_PLAIN,
            1.2,
            color,
            1);
    }

    int raw_width = image_.cols;
    cv::copyMakeBorder(
        image_draw,
        image_draw,
        0,
        0,
        0,
        templ.cols,
        cv::BORDER_CONSTANT,
        cv::Scalar(0, 0, 0));
    cv::Mat draw_templ_roi = image_draw(cv::Rect(raw_width, 0, templ.cols, templ.rows));
    templ.copyTo(draw_templ_roi);

    if (!results.empty()) {
        cv::line(image_draw, cv::Point(raw_width, 0), results.front().box.tl(), color, 1);
    }

    return image_draw;
}

void TemplateMatcher::add_results(ResultsVec results, double threshold)
{
    std::ranges::copy_if(results, std::back_inserter(filtered_results_), [&](const auto& res) {
        return res.score > threshold;
    });

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
        sort_by_score_(results);
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

MAA_VISION_NS_END