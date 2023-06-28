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

    ResultOpt analyze() const;

public:
    static cv::Mat match_template(const cv::Mat& image, const cv::Mat& templ, int method, bool green_mask);

private:
    ResultOpt match_and_postproc(const cv::Mat& templ, double threshold) const;

    TemplMatchingParams param_;
};

MAA_VISION_NS_END
