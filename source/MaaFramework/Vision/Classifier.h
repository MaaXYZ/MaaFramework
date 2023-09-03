#pragma once

#include "Conf/Conf.h"
#include "VisionBase.h"
#include "VisionTypes.h"

MAA_VISION_NS_BEGIN

class Classifier : public VisionBase
{
public:
    struct Result
    {
        size_t cls_index = SIZE_MAX;
        std::string label;
        cv::Rect box {};
        double score = 0.0;
        std::vector<float> raw;
        std::vector<float> probs;
    };
    using ResultOpt = std::optional<Result>;

    void set_param(ClassifierParam param) { param_ = std::move(param); }
    ResultOpt analyze() const;

private:
    Result foreach_rois() const;
    Result classify(const cv::Rect& roi) const;
    void draw_result(const Result& res) const;

    ClassifierParam param_;
};

MAA_VISION_NS_END
