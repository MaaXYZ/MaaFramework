#include "FeatureMatcher.h"

MAA_SUPPRESS_CV_WARNINGS_BEGIN
#include <opencv2/features2d.hpp>
#include <opencv2/opencv.hpp>
#ifdef MAA_VISION_HAS_XFEATURES2D
#include <opencv2/xfeatures2d.hpp>
#endif
MAA_SUPPRESS_CV_WARNINGS_END

#include "Utils/Logger.h"
#include "VisionUtils.hpp"

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

    auto cost = duration_since(start_time);
    LogDebug << name_ << "Raw:" << VAR(results) << VAR(param_.template_path) << VAR(cost);

    int count = param_.count;
    filter(results, count);

    cost = duration_since(start_time);
    LogDebug << name_ << "Filter:" << VAR(results) << VAR(param_.template_path) << VAR(count) << VAR(cost);

    return results;
}

FeatureMatcher::ResultsVec FeatureMatcher::foreach_rois(const cv::Mat& templ) const
{
    if (templ.empty()) {
        LogWarn << name_ << "template is empty" << VAR(param_.template_path);
        return {};
    }

    auto [keypoints_1, descriptors_1] = detect(templ, create_mask(templ, param_.green_mask));

    if (param_.roi.empty()) {
        cv::Rect roi = cv::Rect(0, 0, image_.cols, image_.rows);
        return match_roi(keypoints_1, descriptors_1, roi);
    }

    ResultsVec results;
    for (const cv::Rect& roi : param_.roi) {
        ResultsVec res = match_roi(keypoints_1, descriptors_1, roi);
        results.insert(results.end(), std::make_move_iterator(res.begin()), std::make_move_iterator(res.end()));
    }

    return results;
}

FeatureMatcher::ResultsVec FeatureMatcher::match_roi(const std::vector<cv::KeyPoint>& keypoints_1,
                                                     const cv::Mat& descriptors_1, const cv::Rect& roi_2) const
{
    if (roi_2.empty()) {
        LogError << name_ << "roi_2 is empty";
        return {};
    }

    auto [keypoints_2, descriptors_2] = detect(image_, create_mask(image_, roi_2));

    auto match_points = match(descriptors_1, descriptors_2);

    return postproc(match_points, keypoints_1, keypoints_2, roi_2);
}

cv::Ptr<cv::Feature2D> FeatureMatcher::create_detector() const
{
    switch (param_.detector) {
    case FeatureMatcherParam::Detector::SIFT:
        return cv::SIFT::create();
    case FeatureMatcherParam::Detector::ORB:
        return cv::ORB::create();
    case FeatureMatcherParam::Detector::BRISK:
        return cv::BRISK::create();
    case FeatureMatcherParam::Detector::KAZE:
        return cv::KAZE::create();
    case FeatureMatcherParam::Detector::AKAZE:
        return cv::AKAZE::create();
    case FeatureMatcherParam::Detector::SURF:
#ifdef MAA_VISION_HAS_XFEATURES2D
        return cv::xfeatures2d::SURF::create();
#else
        LogError << name_ << "SURF not enabled";
        return nullptr;
#endif
    }

    LogError << name_ << "Unknown detector" << VAR(static_cast<int>(param_.detector));
    return nullptr;
}

std::pair<std::vector<cv::KeyPoint>, cv::Mat> FeatureMatcher::detect(const cv::Mat& image, const cv::Mat& mask) const
{
    auto detector = create_detector();
    if (!detector) {
        LogError << name_ << "detector is empty";
        return {};
    }

    std::vector<cv::KeyPoint> keypoints;
    cv::Mat descriptors;
    detector->detectAndCompute(image, mask, keypoints, descriptors);

    return std::make_pair(std::move(keypoints), std::move(descriptors));
}

cv::Ptr<cv::DescriptorMatcher> FeatureMatcher::create_matcher() const
{
    switch (param_.detector) {
    case FeatureMatcherParam::Detector::SIFT:
    case FeatureMatcherParam::Detector::SURF:
    case FeatureMatcherParam::Detector::KAZE:
        return cv::FlannBasedMatcher::create();

    case FeatureMatcherParam::Detector::ORB:
    case FeatureMatcherParam::Detector::BRISK:
    case FeatureMatcherParam::Detector::AKAZE:
        return cv::BFMatcher::create(cv::NORM_HAMMING);
    }

    LogError << name_ << "Unknown detector" << VAR(static_cast<int>(param_.detector));
    return nullptr;
}

std::vector<std::vector<cv::DMatch>> FeatureMatcher::match(const cv::Mat& descriptors_1,
                                                           const cv::Mat& descriptors_2) const
{
    if (descriptors_1.empty() || descriptors_2.empty()) {
        LogWarn << name_ << "descriptors is empty";
        return {};
    }

    auto matcher = create_matcher();
    if (!matcher) {
        LogError << name_ << "matcher is empty";
        return {};
    }

    std::vector<cv::Mat> train_desc(1, descriptors_1);
    matcher->add(train_desc);
    matcher->train();

    std::vector<std::vector<cv::DMatch>> match_points;
    matcher->knnMatch(descriptors_2, match_points, 2);
    return match_points;
}

FeatureMatcher::ResultsVec FeatureMatcher::postproc(const std::vector<std::vector<cv::DMatch>>& match_points,
                                                    const std::vector<cv::KeyPoint>& keypoints_1,
                                                    const std::vector<cv::KeyPoint>& keypoints_2,
                                                    const cv::Rect& roi_2) const
{
    std::vector<cv::DMatch> good_matches;
    std::vector<cv::Point2d> obj;
    std::vector<cv::Point2d> scene;

    for (const auto& point : match_points) {
        if (point.size() != 2) {
            continue;
        }

        double threshold = param_.distance_ratio * point[1].distance;
        if (point[0].distance > threshold) {
            continue;
        }
        good_matches.emplace_back(point[0]);
        obj.emplace_back(keypoints_1[point[0].trainIdx].pt);
        scene.emplace_back(keypoints_2[point[0].queryIdx].pt);
    }

    LogDebug << name_ << "Match:" << VAR(good_matches.size()) << VAR(match_points.size()) << VAR(param_.distance_ratio);

    ResultsVec results;
    if (good_matches.size() >= 4) {
        cv::Mat H = cv::findHomography(obj, scene, cv::RANSAC);

        std::array<cv::Point2d, 4> obj_corners = { cv::Point2d(0, 0), cv::Point2d(template_->cols, 0),
                                                   cv::Point2d(template_->cols, template_->rows),
                                                   cv::Point2d(0, template_->rows) };
        std::array<cv::Point2d, 4> scene_corners;
        cv::perspectiveTransform(obj_corners, scene_corners, H);

        double x = std::min({ scene_corners[0].x, scene_corners[1].x, scene_corners[2].x, scene_corners[3].x });
        double y = std::min({ scene_corners[0].y, scene_corners[1].y, scene_corners[2].y, scene_corners[3].y });
        double w = std::max({ scene_corners[0].x, scene_corners[1].x, scene_corners[2].x, scene_corners[3].x }) - x;
        double h = std::max({ scene_corners[0].y, scene_corners[1].y, scene_corners[2].y, scene_corners[3].y }) - y;
        cv::Rect box { static_cast<int>(x), static_cast<int>(y), static_cast<int>(w), static_cast<int>(h) };

        size_t count = MAA_RNS::ranges::count_if(scene, [&box](const auto& point) { return box.contains(point); });

        results.emplace_back(Result { .box = box, .count = static_cast<int>(count) });
    }

    draw_result(*template_, keypoints_1, roi_2, keypoints_2, good_matches, results);

    return results;
}

void FeatureMatcher::draw_result(const cv::Mat& templ, const std::vector<cv::KeyPoint>& keypoints_1,
                                 const cv::Rect& roi, const std::vector<cv::KeyPoint>& keypoints_2,
                                 const std::vector<cv::DMatch>& good_matches, ResultsVec& results) const
{
    if (!debug_draw_) {
        return;
    }

    // const auto color = cv::Scalar(0, 0, 255);
    cv::Mat matches_draw;
    cv::drawMatches(image_, keypoints_2, templ, keypoints_1, good_matches, matches_draw);

    cv::Mat image_draw = draw_roi(roi, matches_draw);
    const auto color = cv::Scalar(0, 0, 255);

    for (size_t i = 0; i != results.size(); ++i) {
        const auto& res = results.at(i);
        cv::rectangle(image_draw, res.box, color, 1);

        std::string flag = MAA_FMT::format("Cnt: {}, [{}, {}, {}, {}]", res.count, res.box.x, res.box.y, res.box.width,
                                           res.box.height);
        cv::putText(image_draw, flag, cv::Point(res.box.x, res.box.y - 5), cv::FONT_HERSHEY_PLAIN, 1.2, color, 1);
    }

    handle_draw(image_draw);
}

void FeatureMatcher::filter(ResultsVec& results, int count) const
{
    auto remove_iter =
        std::remove_if(results.begin(), results.end(), [count](const auto& res) { return res.count < count; });
    results.erase(remove_iter, results.end());
}

MAA_VISION_NS_END
