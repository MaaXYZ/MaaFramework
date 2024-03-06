#pragma once

#include "Utils/JsonExt.hpp"
#include "VisionTypes.h"

MAA_VISION_NS_BEGIN

class TemplateComparator
{
public:
    struct Result
    {
        cv::Rect box {};
        double score = 0.0;

        MEO_JSONIZATION(box, score);
    };
    using ResultsVec = std::vector<Result>;

public:
    TemplateComparator() = default;

    void set_param(TemplateComparatorParam param) { param_ = std::move(param); }
    ResultsVec analyze(const cv::Mat& lhs, const cv::Mat& rhs) const;

private:
    ResultsVec foreach_rois(const cv::Mat& lhs, const cv::Mat& rhs) const;
    void filter(ResultsVec& results, double threshold) const;

    static double comp(const cv::Mat& lhs, const cv::Mat& rhs, int method);

    TemplateComparatorParam param_;
};

MAA_VISION_NS_END
