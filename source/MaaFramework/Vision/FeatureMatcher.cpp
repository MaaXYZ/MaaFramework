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

    auto [keypoints, descriptors] = detect(templ, param_.green_mask);
    auto matcher = create_matcher(keypoints, descriptors);

    if (param_.roi.empty()) {
        return match(matcher, keypoints, cv::Rect(0, 0, image_.cols, image_.rows));
    }

    ResultsVec results;
    for (const cv::Rect& roi : param_.roi) {
        ResultsVec res = match(matcher, keypoints, roi);
        results.insert(results.end(), std::make_move_iterator(res.begin()), std::make_move_iterator(res.end()));
    }

    return results;
}

std::pair<std::vector<cv::KeyPoint>, cv::Mat> FeatureMatcher::detect(const cv::Mat& image, bool green_mask) const
{
    auto detector = cv::xfeatures2d::SURF::create(param_.hessian);

    cv::Mat mask = cv::Mat::ones(image.size(), CV_8UC1);
    if (green_mask) {
        cv::inRange(image, cv::Scalar(0, 255, 0), cv::Scalar(0, 255, 0), mask);
        mask = ~mask;
    }

    std::vector<cv::KeyPoint> keypoints;
    cv::Mat descriptors;
    detector->detectAndCompute(image, mask, keypoints, descriptors);

    return std::make_pair(std::move(keypoints), std::move(descriptors));
}

cv::FlannBasedMatcher FeatureMatcher::create_matcher(const std::vector<cv::KeyPoint>& keypoints,
                                                     const cv::Mat& descriptors) const
{
    std::ignore = keypoints;

    std::vector<cv::Mat> train_desc(1, descriptors);
    cv::FlannBasedMatcher matcher;
    matcher.add(train_desc);
    matcher.train();

    return matcher;
}

FeatureMatcher::ResultsVec FeatureMatcher::match(cv::FlannBasedMatcher& matcher,
                                                 const std::vector<cv::KeyPoint>& keypoints_1,
                                                 const cv::Rect& roi_2) const
{
    auto image_2 = image_with_roi(roi_2);
    auto [keypoints_2, descriptors_2] = detect(image_2, false);

    std::vector<std::vector<cv::DMatch>> match_points;
    matcher.knnMatch(descriptors_2, match_points, 2);

    std::vector<cv::DMatch> good_matches;
    std::vector<cv::Point> good_points;
    for (const auto& point : match_points) {
        if (point.size() != 2) {
            continue;
        }

        double threshold = param_.distance_ratio * point[0].distance;
        if (point[1].distance > threshold) {
            continue;
        }
        good_matches.emplace_back(point[1]);

        cv::Point pt = keypoints_2[point[1].queryIdx].pt;
        good_points.emplace_back(pt);
    }

    draw_result(*template_, keypoints_1, roi_2, keypoints_2, good_matches);

    return {};
}

void FeatureMatcher::draw_result(const cv::Mat& templ, const std::vector<cv::KeyPoint>& keypoints_1,
                                 const cv::Rect& roi, const std::vector<cv::KeyPoint>& keypoints_2,
                                 const std::vector<cv::DMatch>& good_matches) const
{
    if (!debug_draw_) {
        return;
    }

    cv::Mat image_draw = draw_roi(roi);
    // const auto color = cv::Scalar(0, 0, 255);

    cv::drawMatches(templ, keypoints_1, image_draw, keypoints_2, good_matches, image_draw);

    if (save_draw_) {
        save_image(image_draw);
    }
}

void FeatureMatcher::filter(ResultsVec& results, int count) const
{
    std::ignore = results;
    std::ignore = count;
}

MAA_VISION_NS_END
