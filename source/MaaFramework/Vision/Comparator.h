#pragma once

#include "VisionTypes.h"

MAA_VISION_NS_BEGIN

class Comparator
{
public:
    using Result = bool;

public:
    Comparator() = default;

    void set_param(CompParam param) { param_ = std::move(param); }
    Result analyze(const cv::Mat& lhs, const cv::Mat& rhs) const;

private:
    static double comp(const cv::Mat& lhs, const cv::Mat& rhs, int method);

    CompParam param_;
};

MAA_VISION_NS_END
