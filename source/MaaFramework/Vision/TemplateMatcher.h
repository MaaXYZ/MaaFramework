#pragma once

#include <ostream>
#include <vector>

#include "Utils/JsonExt.hpp"
#include "VisionBase.h"
#include "VisionTypes.h"

MAA_VISION_NS_BEGIN

struct TemplateMatcherResult
{
    cv::Rect box {};
    double score = 0.0;

    MEO_JSONIZATION(box, score);
};

class TemplateMatcher
    : public VisionBase
    , public RecoResultAPI<TemplateMatcherResult>
{
public:
    TemplateMatcher(
        cv::Mat image,
        TemplateMatcherParam param,
        std::vector<std::shared_ptr<cv::Mat>> templates,
        std::string name = "");

private:
    void analyze();
    ResultsVec match_all_rois(const cv::Mat& templ) const;
    ResultsVec template_match(const cv::Rect& roi, const cv::Mat& templ) const;

    void add_results(ResultsVec results, double threshold);
    void cherry_pick();

private:
    cv::Mat draw_result(const cv::Rect& roi, const cv::Mat& templ, const ResultsVec& results) const;

    void sort_(ResultsVec& results) const;

private:
    const TemplateMatcherParam param_;
    const std::vector<std::shared_ptr<cv::Mat>> templates_;
};

MAA_VISION_NS_END
