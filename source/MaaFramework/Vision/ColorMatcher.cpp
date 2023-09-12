#include "ColorMatcher.h"

#include "Utils/NoWarningCV.hpp"

#include "Utils/Format.hpp"
#include "Utils/Logger.h"

MAA_VISION_NS_BEGIN

ColorMatcher::ResultsVec ColorMatcher::analyze() const
{
    ResultsVec all_results;

    for (const auto& range : param_.range) {
        auto start_time = std::chrono::steady_clock::now();

        bool connected = param_.connected;
        ResultsVec results = foreach_rois(range, connected);

        auto costs = duration_since(start_time);
        LogDebug << name_ << "Raw:" << VAR(results) << VAR(range.first) << VAR(range.second) << VAR(connected)
                 << VAR(costs);

        int count = param_.count;
        filter(results, count);

        costs = duration_since(start_time);
        LogDebug << name_ << "Filter:" << VAR(results) << VAR(range.first) << VAR(range.second) << VAR(count)
                 << VAR(connected) << VAR(costs);

        all_results.insert(all_results.end(), std::make_move_iterator(results.begin()),
                           std::make_move_iterator(results.end()));
    }

    return all_results;
}

ColorMatcher::ResultsVec ColorMatcher::foreach_rois(const ColorMatcherParam::Range& range, bool connected) const
{
    if (!cache_.empty()) {
        return { color_match(cache_, range, connected) };
    }

    if (param_.roi.empty()) {
        return { color_match(cv::Rect(0, 0, image_.cols, image_.rows), range, connected) };
    }

    ResultsVec res;
    for (const cv::Rect& roi : param_.roi) {
        Result temp = color_match(roi, range, connected);
        res.emplace_back(std::move(temp));
    }

    return res;
}

ColorMatcher::Result ColorMatcher::color_match(const cv::Rect& roi, const ColorMatcherParam::Range& range,
                                               bool connected) const
{
    cv::Mat image = image_with_roi(roi);
    cv::Mat color;
    cv::cvtColor(image, color, param_.method);
    cv::Mat bin;
    cv::inRange(color, range.first, range.second, bin);

    // TODO: 做连通域计算
    std::ignore = connected;

    int count = cv::countNonZero(bin);
    cv::Rect bounding = cv::boundingRect(bin);
    cv::Rect box = bounding + roi.tl();
    cv::Mat dst = bin(bounding);

    Result res { .box = box, .count = count, .dst = dst };

    draw_result(roi, color, res);
    return res;
}

void ColorMatcher::draw_result(const cv::Rect& roi, const cv::Mat& color, const Result& res) const
{
    if (!debug_draw_) {
        return;
    }

    cv::Mat image_draw = draw_roi(roi);
    const auto color_draw = cv::Scalar(0, 0, 255);
    cv::rectangle(image_draw, res.box, color_draw, 1);

    std::string flag =
        MAA_FMT::format("Cnt: {}, [{}, {}, {}, {}]", res.count, res.box.x, res.box.y, res.box.width, res.box.height);
    cv::putText(image_draw, flag, cv::Point(res.box.x, res.box.y - 5), cv::FONT_HERSHEY_PLAIN, 1.2, color_draw, 1);

    int raw_width = image_.cols;
    cv::copyMakeBorder(image_draw, image_draw, 0, 0, 0, color.cols + res.dst.cols, cv::BORDER_CONSTANT,
                       cv::Scalar(0, 0, 0));
    cv::Mat draw_color_roi = image_draw(cv::Rect(raw_width, 0, color.cols, color.rows));
    color.copyTo(draw_color_roi);

    cv::Mat draw_bin_roi = image_draw(cv::Rect(raw_width + color.cols, 0, res.dst.cols, res.dst.rows));
    cv::Mat three_channel_bin;
    cv::cvtColor(res.dst, three_channel_bin, cv::COLOR_GRAY2BGR);
    three_channel_bin.copyTo(draw_bin_roi);

    cv::line(image_draw, cv::Point(raw_width + color.cols, 0), res.box.tl(), color_draw, 1);

    if (save_draw_) {
        save_image(image_draw);
    }
}

void ColorMatcher::filter(ResultsVec& results, int count) const
{
    for (auto iter = results.begin(); iter != results.end();) {
        auto& res = *iter;

        if (res.count < count) {
            iter = results.erase(iter);
            continue;
        }
        ++iter;
    }
}

MAA_VISION_NS_END
