#pragma once

#include <ostream>
#include <vector>

#include "Conf/Conf.h"

MAA_SUPPRESS_CV_WARNINGS_BEGIN
#include <opencv2/features2d.hpp>
MAA_SUPPRESS_CV_WARNINGS_END

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

        operator json::value() const
        {
            json::value root;
            root["box"] = json::array({ box.x, box.y, box.width, box.height });
            root["count"] = count;
            return root;
        }
    };

    using ResultsVec = std::vector<Result>;

public:
    void set_template(std::shared_ptr<cv::Mat> templ) { template_ = std::move(templ); }
    void set_param(FeatureMatcherParam param) { param_ = std::move(param); }

    ResultsVec analyze() const;

private:
    ResultsVec foreach_rois(const cv::Mat& templ) const;
    ResultsVec match_roi(const std::vector<cv::KeyPoint>& keypoints_1, const cv::Mat& descriptors_1,
                         const cv::Rect& roi_2) const;

    cv::Ptr<cv::Feature2D> create_detector() const;
    std::pair<std::vector<cv::KeyPoint>, cv::Mat> detect(const cv::Mat& image, const cv::Mat& mask) const;

    cv::Ptr<cv::DescriptorMatcher> create_matcher() const;
    std::vector<std::vector<cv::DMatch>> match(const cv::Mat& descriptors_1, const cv::Mat& descriptors_2) const;

    ResultsVec postproc(const std::vector<std::vector<cv::DMatch>>& match_points,
                        const std::vector<cv::KeyPoint>& keypoints_1, const std::vector<cv::KeyPoint>& keypoints_2,
                        const cv::Rect& roi_2) const;

    void draw_result(const cv::Mat& templ, const std::vector<cv::KeyPoint>& keypoints_1, const cv::Rect& roi,
                     const std::vector<cv::KeyPoint>& keypoints_2, const std::vector<cv::DMatch>& good_matches,
                     ResultsVec& results) const;
    void filter(ResultsVec& results, int count) const;

    FeatureMatcherParam param_;
    std::shared_ptr<cv::Mat> template_;
};

MAA_VISION_NS_END
