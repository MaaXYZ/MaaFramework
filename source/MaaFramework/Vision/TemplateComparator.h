#pragma once

#include "Utils/JsonExt.hpp"
#include "VisionBase.h"
#include "VisionTypes.h"

MAA_VISION_NS_BEGIN

class TemplateComparator : public VisionBase
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
    TemplateComparator(
        cv::Mat lhs,
        cv::Mat rhs,
        TemplateComparatorParam param,
        std::string name = "");

    const ResultsVec& all_results() const& { return all_results_; }

    ResultsVec&& all_results() && { return std::move(all_results_); }

    const ResultsVec& filtered_results() const& { return filtered_results_; }

    ResultsVec filtered_results() && { return std::move(filtered_results_); }

    const std::optional<Result>& best_result() const& { return best_result_; }

    std::optional<Result> best_result() && { return std::move(best_result_); }

private:
    void analyze();
    ResultsVec compare_all_rois();

    void add_results(ResultsVec results, double threshold);
    void cherry_pick();

    static double comp(const cv::Mat& lhs, const cv::Mat& rhs, int method);

private:
    const cv::Mat rhs_image_ = {};
    const TemplateComparatorParam param_;

private:
    ResultsVec all_results_;
    ResultsVec filtered_results_;
    std::optional<Result> best_result_ = std::nullopt;
};

MAA_VISION_NS_END
