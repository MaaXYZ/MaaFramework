#include "FeatureMatcher.h"

MAA_SUPPRESS_CV_WARNINGS_BEGIN
#include <opencv2/opencv.hpp>
#include <opencv2/xfeatures2d.hpp>
MAA_SUPPRESS_CV_WARNINGS_END

#include "Utils/Logger.h"

MAA_VISION_NS_BEGIN

FeatureMatcher::ResultsVec FeatureMatcher::analyze() const
{
    if (!template_) {
        LogError << name_ << "template_ is empty" << VAR(param_.template_path);
        return {};
    }

    const cv::Mat& templ = *template_;

    auto start_time = std::chrono::steady_clock::now();
    ResultsVec results = foreach_rois(templ);

    auto costs = duration_since(start_time);
    LogDebug << name_ << "Raw:" << VAR(results) << VAR(param_.template_path) << VAR(costs);

    int count = param_.count;
    filter(results, count);

    costs = duration_since(start_time);
    LogDebug << name_ << "Filter:" << VAR(results) << VAR(path) << VAR(count) << VAR(costs);

    return results;
}

FeatureMatcher::ResultsVec FeatureMatcher::foreach_rois(const cv::Mat& templ) const
{
    if (templ.empty()) {
        LogWarn << name_ << "template is empty" << VAR(param_.template_path);
        return {};
    }

    auto matcher = create_matcher(templ);

    if (param_.roi.empty()) {
        return match(cv::Rect(0, 0, image_.cols, image_.rows), matcher);
    }

    ResultsVec results;
    for (const cv::Rect& roi : param_.roi) {
        ResultsVec res = match(roi, matcher);
        results.insert(results.end(), std::make_move_iterator(res.begin()), std::make_move_iterator(res.end()));
    }

    return results;
}

cv::FlannBasedMatcher FeatureMatcher::create_matcher(const cv::Mat& templ) const
{
    std::vector<cv::KeyPoint> keypoints_1;
    cv::Mat descriptors_1;
    detect(templ, param_.green_mask, keypoints_1, descriptors_1);

    std::vector<cv::Mat> train_desc(1, descriptors_1);
    cv::FlannBasedMatcher matcher;
    matcher.add(train_desc);
    matcher.train();

    return matcher;
}

void FeatureMatcher::detect(const cv::Mat& image, bool green_mask, std::vector<cv::KeyPoint>& keypoints,
                            cv::Mat& descriptors) const
{
    auto detector = cv::xfeatures2d::SURF::create(param_.hessian);

    cv::Mat mask = cv::Mat::ones(image.size(), CV_8UC1);
    if (green_mask) {
        cv::inRange(image, cv::Scalar(0, 255, 0), cv::Scalar(0, 255, 0), mask);
        mask = ~mask;
    }

    detector->detectAndCompute(image, mask, keypoints, descriptors);
}

FeatureMatcher::ResultsVec FeatureMatcher::match(const cv::Rect& roi, cv::FlannBasedMatcher& matcher) const
{
    cv::Mat image = image_with_roi(roi);
    std::vector<cv::KeyPoint> keypoints_2;
    cv::Mat descriptors_2;
    detect(image, false, keypoints_2, descriptors_2);

    std::vector<std::vector<cv::DMatch>> match_points;
    matcher.knnMatch(descriptors_2, match_points, 2);

    ResultsVec results;
    for (const auto& point : match_points) {
        if (point.size() != 2) {
            continue;
        }

        if (point[0].distance < param_.distance_ratio * point[1].distance) {
            // TODO
        }
    }
}

void FeatureMatcher::draw_result(const cv::Rect& roi, const ResultsVec& results) const {}

void FeatureMatcher::filter(ResultsVec& results, int count) const {}

MAA_VISION_NS_END
