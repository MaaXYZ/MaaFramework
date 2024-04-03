#pragma once

#include "Utils/JsonExt.hpp"
#include "VisionBase.h"
#include "VisionTypes.h"

MAA_VISION_NS_BEGIN

class ColorMatcher : public VisionBase
{
public:
    struct Result
    {
        cv::Rect box {};
        int count = 0;

        MEO_JSONIZATION(box, count);
    };

    using ResultsVec = std::vector<Result>;

public:
    ColorMatcher(cv::Mat image, ColorMatcherParam param, std::string name = "");

    const ResultsVec& all_results() const& { return all_results_; }

    ResultsVec&& all_results() && { return std::move(all_results_); }

    const ResultsVec& filtered_results() const& { return filtered_results_; }

    ResultsVec filtered_results() && { return std::move(filtered_results_); }

    const std::optional<Result>& best_result() const& { return best_result_; }

    std::optional<Result> best_result() && { return std::move(best_result_); }

private:
    void analyze();
    ResultsVec match_all_rois(const ColorMatcherParam::Range& range);
    ResultsVec color_match(const cv::Rect& roi, const ColorMatcherParam::Range& range);

    void add_results(ResultsVec results, int count);
    void cherry_pick();

private:
    ResultsVec count_non_zero(const cv::Mat& bin, const cv::Point& tl) const;
    ResultsVec count_non_zero_with_connected(const cv::Mat& bin, const cv::Point& tl) const;
    cv::Mat draw_result(
        const cv::Rect& roi,
        const cv::Mat& color,
        const cv::Mat& bin,
        const ResultsVec& results) const;

    void sort_(ResultsVec& results) const;

private:
    const ColorMatcherParam param_;

private:
    ResultsVec all_results_;
    ResultsVec filtered_results_;
    std::optional<Result> best_result_ = std::nullopt;
};

MAA_VISION_NS_END
