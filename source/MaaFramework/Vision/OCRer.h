#pragma once

#include "VisionBase.h"

#include <optional>
#include <vector>

#include "VisionTypes.h"

MAA_VISION_NS_BEGIN

class OCRer : public VisionBase
{
public:
    struct Result
    {
        std::string text;
        cv::Rect box {};
        double score = 0.0;
    };

    using ResultsVec = std::vector<Result>;
    using ResultOpt = std::optional<ResultsVec>;

public:
    using VisionBase::VisionBase;

    void set_param(OcrParam param) { param_ = std::move(param); }
    ResultOpt analyze() const;

private:
    ResultsVec traverse_rois() const;
    ResultsVec predict(const cv::Rect& roi) const;
    ResultsVec predict_det_and_rec(const cv::Rect& roi) const;
    Result predict_only_rec(const cv::Rect& roi) const;
    void postproc_trim_(Result& res) const;
    void postproc_replace_(Result& res) const;
    bool filter_by_required(const Result& res) const;

    OcrParam param_;
};

MAA_VISION_NS_END

std::ostream& operator<<(std::ostream& os, const MAA_VISION_NS::OCRer::Result& res);
