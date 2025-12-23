#include "ColorMatcher.h"

#include <format>

#include "MaaUtils/Logger.h"
#include "MaaUtils/NoWarningCV.hpp"
#include "VisionUtils.hpp"

MAA_VISION_NS_BEGIN

ColorMatcher::ColorMatcher(cv::Mat image, std::vector<cv::Rect> rois, ColorMatcherParam param, std::string name)
    : VisionBase(std::move(image), std::move(rois), std::move(name))
    , param_(std::move(param))
{
    analyze();
}

void ColorMatcher::analyze()
{
    auto start_time = std::chrono::steady_clock::now();

    for (const auto& range : param_.range) {
        while (next_roi()) {
            auto results = color_match(range);
            add_results(std::move(results), param_.count);
        }
        reset_roi();
    }

    cherry_pick();

    auto cost = duration_since(start_time);
    LogDebug << name_ << VAR(all_results_) << VAR(filtered_results_) << VAR(best_result_) << VAR(cost) << VAR(param_.count)
             << VAR(param_.method) << VAR(param_.connected);
}

ColorMatcher::ResultsVec ColorMatcher::color_match(const ColorMatcherParam::Range& range) const
{
    cv::Mat image = image_with_roi();
    cv::Mat color;
    cv::cvtColor(image, color, param_.method);
    cv::Mat bin;
    cv::inRange(color, range.first, range.second, bin);

    ResultsVec results = param_.connected ? count_non_zero_with_connected(bin, roi_.tl()) : count_non_zero(bin, roi_.tl());

    if (debug_draw_) {
        auto draw = draw_result(color, bin, results);
        handle_draw(draw);
    }

    return results;
}

void ColorMatcher::add_results(ResultsVec results, int count)
{
    std::ranges::copy_if(results, std::back_inserter(filtered_results_), [&](const auto& res) { return res.count >= count; });

    merge_vector_(all_results_, std::move(results));
}

void ColorMatcher::cherry_pick()
{
    sort_(all_results_);
    sort_(filtered_results_);

    if (auto index_opt = pythonic_index(filtered_results_.size(), param_.result_index)) {
        best_result_ = filtered_results_.at(*index_opt);
    }
}

ColorMatcher::ResultsVec ColorMatcher::count_non_zero(const cv::Mat& bin, const cv::Point& tl) const
{
    int count = cv::countNonZero(bin);
    cv::Rect bounding = cv::boundingRect(bin);
    cv::Rect box = bounding + tl;

    return { Result { .box = box, .count = count } };
}

ColorMatcher::ResultsVec ColorMatcher::count_non_zero_with_connected(const cv::Mat& bin, const cv::Point& tl) const
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

    return NMS_for_count(std::move(results));
}

cv::Mat ColorMatcher::draw_result(const cv::Mat& color, const cv::Mat& bin, const ResultsVec& results) const
{
    cv::Mat image_draw = draw_roi();
    const auto color_draw = cv::Scalar(0, 0, 255);

    for (size_t i = 0; i < results.size(); ++i) {
        const auto& res = results[i];
        cv::rectangle(image_draw, res.box, color_draw, 1);

        std::string flag = std::format("{}: {}, [{}, {}, {}, {}]", i, res.count, res.box.x, res.box.y, res.box.width, res.box.height);
        cv::putText(image_draw, flag, cv::Point(res.box.x, res.box.y - 5), cv::FONT_HERSHEY_PLAIN, 1.2, color_draw, 1);
        if (i > 10 && res.count < 100) {
            // 太多了画不下，反正后面的也是没用的
            LogDebug << "too many results, skip drawing" << VAR(results.size());
            break;
        }
    }

    int raw_width = image_.cols;
    cv::copyMakeBorder(image_draw, image_draw, 0, 0, 0, color.cols + bin.cols, cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));
    cv::Mat draw_color_roi = image_draw(cv::Rect(raw_width, 0, color.cols, color.rows));
    color.copyTo(draw_color_roi);

    cv::Mat draw_bin_roi = image_draw(cv::Rect(raw_width + color.cols, 0, bin.cols, bin.rows));
    cv::Mat three_channel_bin;
    cv::cvtColor(bin, three_channel_bin, cv::COLOR_GRAY2BGR);
    three_channel_bin.copyTo(draw_bin_roi);

    // cv::line(image_draw, cv::Point(raw_width + color.cols, 0), res.box.tl(), color_draw, 1);

    return image_draw;
}

void ColorMatcher::sort_(ResultsVec& results) const
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

MAA_VISION_NS_END
