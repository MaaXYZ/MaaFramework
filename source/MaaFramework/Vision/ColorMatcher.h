#pragma once

#include "Utils/JsonExt.hpp"
#include "VisionBase.h"
#include "VisionTypes.h"

MAA_VISION_NS_BEGIN

class ColorMatcher : public VisionBase
{
public:
    struct Result
    {
        cv::Rect box {};
        int count = 0;

        MEO_JSONIZATION(box, count);
    };

    using ResultsVec = std::vector<Result>;

public:
    ColorMatcher(ColorMatcherParam param, cv::Mat image, std::string name);

    void analyze();

    const ResultsVec& raw_results() const { return raw_results_; }

    const ResultsVec& filtered_results() const { return filtered_results_; }

private:
    ResultsVec foreach_rois(const ColorMatcherParam::Range& range, bool connected);
    ResultsVec
        color_match(const cv::Rect& roi, const ColorMatcherParam::Range& range, bool connected);
    ResultsVec count_non_zero(const cv::Mat& bin, const cv::Point& tl) const;
    ResultsVec count_non_zero_with_connected(const cv::Mat& bin, const cv::Point& tl) const;
    cv::Mat draw_result(
        const cv::Rect& roi,
        const cv::Mat& color,
        const cv::Mat& bin,
        const ResultsVec& results) const;

    void filter(ResultsVec& results, int count) const;
    void sort(ResultsVec& results) const;

private:
    const ColorMatcherParam param_;

    bool analyzed_ = false;
    ResultsVec raw_results_;
    ResultsVec filtered_results_;
};

MAA_VISION_NS_END
