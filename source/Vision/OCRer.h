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

    ResultOpt analyze() const;

private:
    ResultsVec predict() const;
    void postproc_trim_(Result& res) const;
    void postproc_replace_(Result& res) const;

    OcrParams param_;
};

MAA_VISION_NS_END

std::ostream& operator<<(std::ostream& os, const MAA_VISION_NS::OCRer::Result& res);
