#pragma once

#include "MaaUtils/JsonExt.hpp"
#include "VisionBase.h"
#include "VisionTypes.h"

#include "Common/Conf.h"

MAA_VISION_NS_BEGIN

struct MultiColorMatcherResult
{
    cv::Rect box { }; // base point position (1x1)
    int count = 1;

    MEO_JSONIZATION(box, count);
};

class MultiColorMatcher
    : public VisionBase
    , public RecoResultAPI<MultiColorMatcherResult>
{
public:
    MultiColorMatcher(cv::Mat image, std::vector<cv::Rect> rois, MultiColorMatcherParam param, std::string name = "");

private:
    void analyze();
    ResultsVec multi_color_match() const;

    void add_results(ResultsVec results);
    void cherry_pick();

private:
    // Unpack 0xRRGGBB into R/G/B channels
    static cv::Scalar unpack_color(int color);
    // Tolerance: if dev (0xRRGGBB) is non-zero, unpack per channel; otherwise use 100 - degree
    static cv::Scalar tolerance(int dev, int degree);
    // Translate mask: result(x, y) = mask(x + dx, y + dy)
    static cv::Mat translate_mask(const cv::Mat& mask, int dx, int dy);
    // Cluster by miss distance: keep the first point of each cluster
    static ResultsVec cluster_miss(const ResultsVec& results, int miss);
    // Sort by orient (affects which point is kept during clustering)
    static void sort_by_orient(ResultsVec& results, int orient);

    cv::Mat draw_result(const ResultsVec& results) const;

    void sort_(ResultsVec& results) const;

private:
    const MultiColorMatcherParam param_;
};

MAA_VISION_NS_END
