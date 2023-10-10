#pragma once

#include <ostream>
#include <vector>

#include <opencv2/features2d.hpp>

#include "VisionBase.h"
#include "VisionTypes.h"

MAA_VISION_NS_BEGIN

class FeatureMatcher : public VisionBase
{
public:
    struct Result
    {
        cv::Rect box {};
        double score = 0.0;

        json::value to_json() const
        {
            json::value root;
            root["box"] = json::array({ box.x, box.y, box.width, box.height });
            root["score"] = score;
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
    cv::FlannBasedMatcher create_matcher(const cv::Mat& templ) const;
    void detect(const cv::Mat& image, bool green_mask, std::vector<cv::KeyPoint>& keypoints,
                cv::Mat& descriptors) const;
    ResultsVec match(const cv::Rect& roi, cv::FlannBasedMatcher& matcher) const;
    void draw_result(const cv::Rect& roi, const ResultsVec& results) const;
    void filter(ResultsVec& results, int count) const;

    FeatureMatcherParam param_;
    std::shared_ptr<cv::Mat> template_;
};

MAA_VISION_NS_END
