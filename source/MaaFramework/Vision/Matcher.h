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

        json::value to_json() const
        {
            json::value root;
            root["box"] = json::array({ box.x, box.y, box.width, box.height });
            root["score"] = score;
            return root;
        }
    };
    using ResultOpt = std::optional<Result>;

public:
    using VisionBase::VisionBase;

    void set_param(TemplMatchingParam param) { param_ = std::move(param); }
    ResultOpt analyze() const;

private:
    Result traverse_rois(const cv::Mat& templ, double threshold) const;
    Result match_and_postproc(const cv::Rect& roi, const cv::Mat& templ) const;

    TemplMatchingParam param_;
};

MAA_VISION_NS_END
