#pragma once

#include <ostream>
#include <vector>

#include "Utils/JsonExt.hpp"
#include "VisionBase.h"
#include "VisionTypes.h"

MAA_VISION_NS_BEGIN

class TemplateMatcher : public VisionBase
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
    void set_templates(std::vector<std::shared_ptr<cv::Mat>> templates) { templates_ = std::move(templates); }
    void set_param(TemplateMatcherParam param) { param_ = std::move(param); }
    ResultsVec analyze() const;

private:
    ResultsVec foreach_rois(const cv::Mat& templ) const;
    ResultsVec match(const cv::Rect& roi, const cv::Mat& templ) const;
    void draw_result(const cv::Rect& roi, const cv::Mat& templ, const ResultsVec& results) const;

    void filter(ResultsVec& results, double threshold) const;

    TemplateMatcherParam param_;
    std::vector<std::shared_ptr<cv::Mat>> templates_;
};

MAA_VISION_NS_END
