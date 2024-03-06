#include "TemplateMatcher.h"

#include "Utils/Logger.h"
#include "Utils/NoWarningCV.hpp"
#include "Utils/StringMisc.hpp"
#include "VisionUtils.hpp"

MAA_VISION_NS_BEGIN

std::pair<TemplateMatcher::ResultsVec, size_t> TemplateMatcher::analyze() const
{
    if (templates_.empty()) {
        LogError << name_ << "templates is empty" << VAR(param_.template_paths);
        return {};
    }

    if (templates_.size() != param_.thresholds.size()) {
        LogError << name_ << "templates.size() != thresholds.size()" << VAR(templates_.size())
                 << VAR(param_.thresholds.size());
        return {};
    }

    ResultsVec all_results;
    for (size_t i = 0; i != templates_.size(); ++i) {
        const auto& image_ptr = templates_.at(i);
        if (!image_ptr) {
            LogWarn << name_ << "template is empty" << VAR(param_.template_paths.at(i))
                    << VAR(image_ptr);
            continue;
        }

        const cv::Mat& templ = *image_ptr;

        auto start_time = std::chrono::steady_clock::now();
        ResultsVec results = foreach_rois(templ);

        auto cost = duration_since(start_time);
        const std::string& path = param_.template_paths.at(i);
        LogTrace << name_ << "Raw:" << VAR(results) << VAR(path) << VAR(cost);

        double threshold = param_.thresholds.at(i);
        filter(results, threshold);

        cost = duration_since(start_time);
        LogTrace << name_ << "Filter:" << VAR(results) << VAR(path) << VAR(threshold) << VAR(cost);

        all_results.insert(
            all_results.end(),
            std::make_move_iterator(results.begin()),
            std::make_move_iterator(results.end()));
    }

    sort(all_results);
    size_t index = preferred_index(all_results);

    return { all_results, index };
}

TemplateMatcher::ResultsVec TemplateMatcher::foreach_rois(const cv::Mat& templ) const
{
    if (templ.empty()) {
        LogWarn << name_ << "template is empty" << VAR(param_.template_paths) << VAR(templ.size());
        return {};
    }

    if (param_.roi.empty()) {
        return match(cv::Rect(0, 0, image_.cols, image_.rows), templ);
    }

    ResultsVec results;
    for (const cv::Rect& roi : param_.roi) {
        ResultsVec res = match(roi, templ);
        results.insert(
            results.end(),
            std::make_move_iterator(res.begin()),
            std::make_move_iterator(res.end()));
    }

    return results;
}

TemplateMatcher::ResultsVec TemplateMatcher::match(const cv::Rect& roi, const cv::Mat& templ) const
{
    cv::Mat image = image_with_roi(roi);

    if (templ.cols > image.cols || templ.rows > image.rows) {
        LogError << name_ << "templ size is too large" << VAR(image) << VAR(templ);
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
    draw_result(roi, templ, nms_results);

    return nms_results;
}

void TemplateMatcher::draw_result(
    const cv::Rect& roi,
    const cv::Mat& templ,
    const ResultsVec& results) const
{
    if (!debug_draw_) {
        return;
    }

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

    handle_draw(image_draw);
}

void TemplateMatcher::filter(ResultsVec& results, double threshold) const
{
    std::erase_if(results, [threshold](const auto& res) { return res.score < threshold; });
}

void TemplateMatcher::sort(ResultsVec& results) const
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

size_t TemplateMatcher::preferred_index(const ResultsVec& results) const
{
    auto index_opt = pythonic_index(results.size(), param_.result_index);
    if (!index_opt) {
        return SIZE_MAX;
    }

    return *index_opt;
}

MAA_VISION_NS_END