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
    TemplateComparator(cv::Mat lhs, cv::Mat rhs, cv::Rect roi, TemplateComparatorParam param, std::string name = "");

private:
    void analyze();

    void add_results(ResultsVec results, double threshold);
    void cherry_pick();

    double comp(const cv::Mat& lhs, const cv::Mat& rhs, int method);
    bool comp_score(double s1, double s2) const;

private:
    const cv::Mat rhs_image_ = {};
    const TemplateComparatorParam param_;
    const bool use_min_score_ = false;
};

MAA_VISION_NS_END
