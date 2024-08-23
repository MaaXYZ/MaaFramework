#pragma once

#include "Utils/JsonExt.hpp"
#include "VisionBase.h"
#include "VisionTypes.h"

MAA_VISION_NS_BEGIN

struct ColorMatcherResult
{
    cv::Rect box {};
    int count = 0;

    MEO_JSONIZATION(box, count);
};

class ColorMatcher
    : public VisionBase
    , public RecoResultAPI<ColorMatcherResult>
{
public:
    ColorMatcher(cv::Mat image, ColorMatcherParam param, std::string name = "");

private:
    void analyze();
    ResultsVec match_all_rois(const ColorMatcherParam::Range& range) const;
    ResultsVec color_match(const cv::Rect& roi, const ColorMatcherParam::Range& range) const;

    void add_results(ResultsVec results, int count);
    void cherry_pick();

private:
    ResultsVec count_non_zero(const cv::Mat& bin, const cv::Point& tl) const;
    ResultsVec count_non_zero_with_connected(const cv::Mat& bin, const cv::Point& tl) const;
    cv::Mat draw_result(const cv::Rect& roi, const cv::Mat& color, const cv::Mat& bin, const ResultsVec& results) const;

    void sort_(ResultsVec& results) const;

private:
    const ColorMatcherParam param_;
};

MAA_VISION_NS_END
