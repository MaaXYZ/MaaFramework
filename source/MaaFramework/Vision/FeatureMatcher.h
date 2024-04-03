#pragma once

#include <ostream>
#include <vector>

#include "Conf/Conf.h"

MAA_SUPPRESS_CV_WARNINGS_BEGIN
#include <opencv2/features2d.hpp>
MAA_SUPPRESS_CV_WARNINGS_END

#include "Utils/JsonExt.hpp"
#include "VisionBase.h"
#include "VisionTypes.h"

MAA_VISION_NS_BEGIN

class FeatureMatcher : public VisionBase
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
    FeatureMatcher(
        cv::Mat image,
        FeatureMatcherParam param,
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
    ResultsVec feature_match(
        const cv::Mat& templ,
        const std::vector<cv::KeyPoint>& keypoints_1,
        const cv::Mat& descriptors_1,
        const cv::Rect& roi_2);

    void add_results(ResultsVec results, int count);
    void cherry_pick();

private:
    cv::Ptr<cv::Feature2D> create_detector() const;
    std::pair<std::vector<cv::KeyPoint>, cv::Mat>
        detect(const cv::Mat& image, const cv::Mat& mask) const;

    cv::Ptr<cv::DescriptorMatcher> create_matcher() const;
    std::vector<std::vector<cv::DMatch>>
        match(const cv::Mat& descriptors_1, const cv::Mat& descriptors_2) const;

    ResultsVec feature_postproc(
        const std::vector<std::vector<cv::DMatch>>& match_points,
        const std::vector<cv::KeyPoint>& keypoints_1,
        const std::vector<cv::KeyPoint>& keypoints_2,
        int templ_cols,
        int templ_rows,
        std::vector<cv::DMatch>& good_matches) const;

    cv::Mat draw_result(
        const cv::Mat& templ,
        const std::vector<cv::KeyPoint>& keypoints_1,
        const cv::Rect& roi,
        const std::vector<cv::KeyPoint>& keypoints_2,
        const std::vector<cv::DMatch>& good_matches,
        const ResultsVec& results) const;

    void sort_(ResultsVec& results) const;

private:
    const FeatureMatcherParam param_;
    const std::vector<std::shared_ptr<cv::Mat>> templates_;

private:
    ResultsVec all_results_;
    ResultsVec filtered_results_;
    std::optional<Result> best_result_ = std::nullopt;
};

MAA_VISION_NS_END
