#pragma once

#include "VisionBase.h"

#include <optional>

#include "VisionTypes.h"

MAA_VISION_NS_BEGIN

class Matcher : public VisionBase
{
public:
    struct Result
    {
        cv::Rect box {};
        double score = 0.0;
    };
    using ResultOpt = std::optional<Result>;

public:
    using VisionBase::VisionBase;

    void set_param(TemplMatchingParams param) { param_ = std::move(param); }
    ResultOpt analyze() const;

private:
    ResultOpt traverse_rois(const cv::Mat& templ, double threshold) const;
    ResultOpt match_and_postproc(const cv::Rect& roi, const cv::Mat& templ, double threshold) const;

    TemplMatchingParams param_;
};

MAA_VISION_NS_END
