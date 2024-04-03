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
    TemplateMatcher(
        cv::Mat image,
        TemplateMatcherParam param,
        std::vector<std::shared_ptr<cv::Mat>> templates,
        std::string name = "");

    const ResultsVec& all_results() const& { return all_results_; }

    ResultsVec&& all_results() && { return std::move(all_results_); }

    const ResultsVec& filtered_results() const& { return filtered_results_; }

    ResultsVec filtered_results() && { return std::move(filtered_results_); }

    const std::optional<Result>& best_result() const& { return best_result_; }

    std::optional<Result> best_result() && { return std::move(best_result_); }

private:
    void analyze();
    ResultsVec match_all_rois(const cv::Mat& templ);
    ResultsVec template_match(const cv::Rect& roi, const cv::Mat& templ);

    void add_results(ResultsVec results, double threshold);
    void cherry_pick();

private:
    cv::Mat draw_result(const cv::Rect& roi, const cv::Mat& templ, const ResultsVec& results) const;

    void sort_(ResultsVec& results) const;

private:
    const TemplateMatcherParam param_;
    const std::vector<std::shared_ptr<cv::Mat>> templates_;

private:
    ResultsVec all_results_;
    ResultsVec filtered_results_;
    std::optional<Result> best_result_ = std::nullopt;
};

MAA_VISION_NS_END
