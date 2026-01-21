#pragma once

#include <ostream>
#include <vector>

#include "Common/Conf.h"

MAA_SUPPRESS_CV_WARNINGS_BEGIN
#include <opencv2/features2d.hpp>
MAA_SUPPRESS_CV_WARNINGS_END

#include "MaaUtils/JsonExt.hpp"
#include "VisionBase.h"
#include "VisionTypes.h"

MAA_VISION_NS_BEGIN

struct FeatureMatcherResult
{
    cv::Rect box {};
    int count = 0;

    MEO_JSONIZATION(box, count);
};

class FeatureMatcher
    : public VisionBase
    , public RecoResultAPI<FeatureMatcherResult>
{
public:
    FeatureMatcher(
        cv::Mat image,
        std::vector<cv::Rect> rois,
        FeatureMatcherParam param,
        std::vector<cv::Mat> templates,
        std::string name = "");

private:
    void analyze();
    ResultsVec feature_match(const cv::Mat& templ, const std::vector<cv::KeyPoint>& keypoints_1, const cv::Mat& descriptors_1) const;

    void add_results(ResultsVec results, int count);
    void cherry_pick();

private:
    cv::Ptr<cv::Feature2D> create_detector() const;
    std::pair<std::vector<cv::KeyPoint>, cv::Mat> detect(const cv::Mat& image, const cv::Mat& mask) const;

    cv::Ptr<cv::DescriptorMatcher> create_matcher() const;
    std::vector<std::vector<cv::DMatch>> match(const cv::Mat& descriptors_1, const cv::Mat& descriptors_2) const;

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
        const std::vector<cv::KeyPoint>& keypoints_2,
        const std::vector<cv::DMatch>& good_matches,
        const ResultsVec& results) const;

    void sort_(ResultsVec& results) const;

private:
    const FeatureMatcherParam param_;
    const std::vector<cv::Mat> templates_;
};

MAA_VISION_NS_END
