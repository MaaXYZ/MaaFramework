#include "ColorMatcher.h"

#include <format>

#include "Utils/Logger.h"
#include "Utils/NoWarningCV.hpp"
#include "VisionUtils.hpp"

MAA_VISION_NS_BEGIN

std::pair<ColorMatcher::ResultsVec, size_t> ColorMatcher::analyze() const
{
    ResultsVec all_results;

    for (const auto& range : param_.range) {
        auto start_time = std::chrono::steady_clock::now();

        bool connected = param_.connected;
        ResultsVec results = foreach_rois(range, connected);

        auto cost = duration_since(start_time);
        LogTrace << name_ << "Raw:" << VAR(results) << VAR(range.first) << VAR(range.second)
                 << VAR(connected) << VAR(cost);

        int count = param_.count;
        filter(results, count);

        cost = duration_since(start_time);
        LogTrace << name_ << "Filter:" << VAR(results) << VAR(range.first) << VAR(range.second)
                 << VAR(count) << VAR(connected) << VAR(cost);

        all_results.insert(
            all_results.end(),
            std::make_move_iterator(results.begin()),
            std::make_move_iterator(results.end()));
    }

    sort(all_results);
    size_t index = preferred_index(all_results);

    return { all_results, index };
}

ColorMatcher::ResultsVec
    ColorMatcher::foreach_rois(const ColorMatcherParam::Range& range, bool connected) const
{
    if (param_.roi.empty()) {
        return { color_match(cv::Rect(0, 0, image_.cols, image_.rows), range, connected) };
    }

    ResultsVec results;
    for (const cv::Rect& roi : param_.roi) {
        ResultsVec res = color_match(roi, range, connected);
        results.insert(
            results.end(),
            std::make_move_iterator(res.begin()),
            std::make_move_iterator(res.end()));
    }

    return results;
}

ColorMatcher::ResultsVec ColorMatcher::color_match(
    const cv::Rect& roi,
    const ColorMatcherParam::Range& range,
    bool connected) const
{
    cv::Mat image = image_with_roi(roi);
    cv::Mat color;
    cv::cvtColor(image, color, param_.method);
    cv::Mat bin;
    cv::inRange(color, range.first, range.second, bin);

    ResultsVec results =
        connected ? count_non_zero_with_connected(bin, roi.tl()) : count_non_zero(bin, roi.tl());

    draw_result(roi, color, bin, results);
    return results;
}

ColorMatcher::ResultsVec ColorMatcher::count_non_zero(const cv::Mat& bin, const cv::Point& tl) const
{
    int count = cv::countNonZero(bin);
    cv::Rect bounding = cv::boundingRect(bin);
    cv::Rect box = bounding + tl;

    return { Result { .box = box, .count = count } };
}

ColorMatcher::ResultsVec
    ColorMatcher::count_non_zero_with_connected(const cv::Mat& bin, const cv::Point& tl) const
{
    ResultsVec results;

    cv::Mat labels, stats, centroids;
    int number = cv::connectedComponentsWithStats(bin, labels, stats, centroids, 8, CV_16U);
    for (int i = 1; i < number; ++i) {
        // int center_x = centroids.at<double>(i, 0);
        // int center_y = centroids.at<double>(i, 1);

        int x = stats.at<int>(i, cv::CC_STAT_LEFT);
        int y = stats.at<int>(i, cv::CC_STAT_TOP);
        int width = stats.at<int>(i, cv::CC_STAT_WIDTH);
        int height = stats.at<int>(i, cv::CC_STAT_HEIGHT);

        cv::Rect bounding = cv::Rect(x, y, width, height);
        // int count = stats.at<int>(i, cv::CC_STAT_AREA);
        int count = cv::countNonZero(bin(bounding));

        Result res { .box = bounding + tl, .count = count };
        results.emplace_back(std::move(res));
    }

    return NMS_for_count(std::move(results), 0.7);
}

void ColorMatcher::draw_result(
    const cv::Rect& roi,
    const cv::Mat& color,
    const cv::Mat& bin,
    const ResultsVec& results) const
{
    if (!debug_draw_) {
        return;
    }

    cv::Mat image_draw = draw_roi(roi);
    const auto color_draw = cv::Scalar(0, 0, 255);

    for (size_t i = 0; i < results.size(); ++i) {
        const auto& res = results[i];
        cv::rectangle(image_draw, res.box, color_draw, 1);

        std::string flag = std::format(
            "{}: {}, [{}, {}, {}, {}]",
            i,
            res.count,
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
            color_draw,
            1);
        if (i > 10 && res.count < 100) {
            // 太多了画不下，反正后面的也是没用的
            LogDebug << "too many results, skip drawing" << VAR(results.size());
            break;
        }
    }

    int raw_width = image_.cols;
    cv::copyMakeBorder(
        image_draw,
        image_draw,
        0,
        0,
        0,
        color.cols + bin.cols,
        cv::BORDER_CONSTANT,
        cv::Scalar(0, 0, 0));
    cv::Mat draw_color_roi = image_draw(cv::Rect(raw_width, 0, color.cols, color.rows));
    color.copyTo(draw_color_roi);

    cv::Mat draw_bin_roi = image_draw(cv::Rect(raw_width + color.cols, 0, bin.cols, bin.rows));
    cv::Mat three_channel_bin;
    cv::cvtColor(bin, three_channel_bin, cv::COLOR_GRAY2BGR);
    three_channel_bin.copyTo(draw_bin_roi);

    // cv::line(image_draw, cv::Point(raw_width + color.cols, 0), res.box.tl(), color_draw, 1);

    handle_draw(image_draw);
}

void ColorMatcher::filter(ResultsVec& results, int count) const
{
    std::erase_if(results, [count](const auto& res) { return res.count < count; });
}

void ColorMatcher::sort(ResultsVec& results) const
{
    switch (param_.order_by) {
    case ResultOrderBy::Horizontal:
        sort_by_horizontal_(results);
        break;
    case ResultOrderBy::Vertical:
        sort_by_vertical_(results);
        break;
    case ResultOrderBy::Score:
        sort_by_count_(results);
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

size_t ColorMatcher::preferred_index(const ResultsVec& results) const
{
    auto index_opt = pythonic_index(results.size(), param_.result_index);
    if (!index_opt) {
        return SIZE_MAX;
    }

    return *index_opt;
}

MAA_VISION_NS_END