#pragma once

#include "Utils/JsonExt.hpp"
#include "VisionBase.h"
#include "VisionTypes.h"

MAA_VISION_NS_BEGIN

struct TemplateComparatorResult
{
    cv::Rect box {};
    double score = 0.0;

    MEO_JSONIZATION(box, score);
};

class TemplateComparator
    : public VisionBase
    , public RecoResultAPI<TemplateComparatorResult>
{
public:
    TemplateComparator(
        cv::Mat lhs,
        cv::Mat rhs,
        TemplateComparatorParam param,
        std::string name = "");

private:
    void analyze();
    ResultsVec compare_all_rois() const;

    void add_results(ResultsVec results, double threshold);
    void cherry_pick();

    static double comp(const cv::Mat& lhs, const cv::Mat& rhs, int method);

private:
    const cv::Mat rhs_image_ = {};
    const TemplateComparatorParam param_;
};

MAA_VISION_NS_END
