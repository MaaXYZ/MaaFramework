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
    LogDebug << name_ << "Filter:" << VAR(results) << VAR(param_.template_path) << VAR(count) << VAR(costs);

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

std::pair<std::vector<cv::KeyPoint>, cv::Mat> FeatureMatcher::detect(const cv::Mat& image, const cv::Rect& roi) const
{
    auto detector = cv::xfeatures2d::SURF::create(param_.hessian);

    cv::Mat mask = cv::Mat::zeros(image.size(), CV_8UC1);
    mask(roi) = 255;

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
    auto [keypoints_2, descriptors_2] = detect(image_, roi_2);

    std::vector<std::vector<cv::DMatch>> match_points;
    matcher.knnMatch(descriptors_2, match_points, 2);

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
    if (!good_matches.empty()) {
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

        std::string flag = MAA_FMT::format("{}: {}, [{}, {}, {}, {}]", i, res.count, res.box.x, res.box.y,
                                           res.box.width, res.box.height);
        cv::putText(image_draw, flag, cv::Point(res.box.x, res.box.y - 5), cv::FONT_HERSHEY_PLAIN, 1.2, color, 1);
    }

    if (save_draw_) {
        save_image(image_draw);
    }
}

void FeatureMatcher::filter(ResultsVec& results, int count) const
{
    for (auto iter = results.begin(); iter != results.end();) {
        auto& res = *iter;

        if (res.count < count) {
            iter = results.erase(iter);
            continue;
        }
        ++iter;
    }
}

MAA_VISION_NS_END
