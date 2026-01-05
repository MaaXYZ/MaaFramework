#pragma once

#include <ostream>
#include <vector>

#include "MaaUtils/JsonExt.hpp"
#include "VisionBase.h"
#include "VisionTypes.h"

#include "Common/Conf.h"

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
        std::vector<cv::Rect> rois,
        TemplateMatcherParam param,
        std::vector<cv::Mat> templates,
        std::string name = "");

private:
    void analyze();
    ResultsVec template_match(const cv::Mat& templ) const;

    void add_results(ResultsVec results, double threshold);
    void cherry_pick();

private:
    cv::Mat draw_result(const cv::Mat& templ, const ResultsVec& results) const;

    void sort_(ResultsVec& results) const;

    bool comp_score(double s1, double s2) const;

private:
    const TemplateMatcherParam param_;
    const bool low_score_better_ = false;
    const std::vector<cv::Mat> templates_;
};

MAA_VISION_NS_END
