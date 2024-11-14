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

FeatureMatcher::FeatureMatcher(
    cv::Mat image,
    cv::Rect roi,
    FeatureMatcherParam param,
    std::vector<std::shared_ptr<cv::Mat>> templates,
    std::string name)
    : VisionBase(std::move(image), std::move(roi), std::move(name))
    , param_(std::move(param))
    , templates_(std::move(templates))
{
    analyze();
}

void FeatureMatcher::analyze()
{
    if (templates_.empty()) {
        LogError << name_ << VAR(uid_) << "templates is empty" << VAR(param_.template_paths);
        return;
    }

    auto start_time = std::chrono::steady_clock::now();

    for (const auto& templ_ptr : templates_) {
        if (!templ_ptr) {
            continue;
        }
        const auto& templ = *templ_ptr;

        auto [keypoints_1, descriptors_1] = detect(templ, create_mask(templ, param_.green_mask));

        auto results = feature_match(templ, keypoints_1, descriptors_1);
        add_results(std::move(results), param_.count);
    }

    cherry_pick();

    auto cost = duration_since(start_time);
    LogDebug << name_ << VAR(uid_) << VAR(all_results_) << VAR(filtered_results_) << VAR(best_result_) << VAR(cost)
             << VAR(param_.template_paths) << VAR(param_.green_mask) << VAR(param_.distance_ratio) << VAR(param_.count);
}

FeatureMatcher::ResultsVec
    FeatureMatcher::feature_match(const cv::Mat& templ, const std::vector<cv::KeyPoint>& keypoints_1, const cv::Mat& descriptors_1) const
{
    auto [keypoints_2, descriptors_2] = detect(image_, create_mask(image_, roi_));

    auto match_points = match(descriptors_1, descriptors_2);

    std::vector<cv::DMatch> good_matches;
    ResultsVec results = feature_postproc(match_points, keypoints_1, keypoints_2, templ.cols, templ.rows, good_matches);

    if (debug_draw_) {
        auto draw = draw_result(templ, keypoints_1, keypoints_2, good_matches, results);
        handle_draw(draw);
    }

    return results;
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
        LogError << name_ << VAR(uid_) << "SURF not enabled";
        return nullptr;
#endif
    }

    LogError << name_ << VAR(uid_) << "Unknown detector" << VAR(static_cast<int>(param_.detector));
    return nullptr;
}

std::pair<std::vector<cv::KeyPoint>, cv::Mat> FeatureMatcher::detect(const cv::Mat& image, const cv::Mat& mask) const
{
    auto detector = create_detector();
    if (!detector) {
        LogError << name_ << VAR(uid_) << "detector is empty";
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

    LogError << name_ << VAR(uid_) << "Unknown detector" << VAR(static_cast<int>(param_.detector));
    return nullptr;
}

std::vector<std::vector<cv::DMatch>> FeatureMatcher::match(const cv::Mat& descriptors_1, const cv::Mat& descriptors_2) const
{
    if (descriptors_1.empty() || descriptors_2.empty()) {
        LogWarn << name_ << "descriptors is empty";
        return {};
    }

    auto matcher = create_matcher();
    if (!matcher) {
        LogError << name_ << VAR(uid_) << "matcher is empty";
        return {};
    }

    std::vector<cv::Mat> train_desc(1, descriptors_1);
    matcher->add(train_desc);
    matcher->train();

    std::vector<std::vector<cv::DMatch>> match_points;
    matcher->knnMatch(descriptors_2, match_points, 2);
    return match_points;
}

FeatureMatcher::ResultsVec FeatureMatcher::feature_postproc(
    const std::vector<std::vector<cv::DMatch>>& match_points,
    const std::vector<cv::KeyPoint>& keypoints_1,
    const std::vector<cv::KeyPoint>& keypoints_2,
    int templ_cols,
    int templ_rows,
    std::vector<cv::DMatch>& good_matches) const
{
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

    LogDebug << name_ << VAR(uid_) << "Match:" << VAR(good_matches.size()) << VAR(match_points.size()) << VAR(param_.distance_ratio);

    if (good_matches.size() < 4) {
        return {};
    }

    cv::Mat homography = cv::findHomography(obj, scene, cv::RHO);

    if (homography.empty()) {
        LogDebug << name_ << VAR(uid_) << "Homography is empty";
        return {};
    }

    std::array<cv::Point2d, 4> obj_corners = { cv::Point2d(0, 0),
                                               cv::Point2d(templ_cols, 0),
                                               cv::Point2d(templ_cols, templ_rows),
                                               cv::Point2d(0, templ_rows) };
    std::array<cv::Point2d, 4> scene_corners;
    cv::perspectiveTransform(obj_corners, scene_corners, homography);

    double x = std::min({ scene_corners[0].x, scene_corners[1].x, scene_corners[2].x, scene_corners[3].x });
    double y = std::min({ scene_corners[0].y, scene_corners[1].y, scene_corners[2].y, scene_corners[3].y });
    double w = std::max({ scene_corners[0].x, scene_corners[1].x, scene_corners[2].x, scene_corners[3].x }) - x;
    double h = std::max({ scene_corners[0].y, scene_corners[1].y, scene_corners[2].y, scene_corners[3].y }) - y;
    cv::Rect box { static_cast<int>(x), static_cast<int>(y), static_cast<int>(w), static_cast<int>(h) };
    box &= roi_;

    size_t count = std::ranges::count_if(scene, [&box](const auto& point) { return box.contains(point); });

    return { Result { .box = box, .count = static_cast<int>(count) } };
}

cv::Mat FeatureMatcher::draw_result(
    const cv::Mat& templ,
    const std::vector<cv::KeyPoint>& keypoints_1,
    const std::vector<cv::KeyPoint>& keypoints_2,
    const std::vector<cv::DMatch>& good_matches,
    const ResultsVec& results) const
{
    // const auto color = cv::Scalar(0, 0, 255);
    cv::Mat matches_draw;
    cv::drawMatches(image_, keypoints_2, templ, keypoints_1, good_matches, matches_draw);

    cv::Mat image_draw = draw_roi(matches_draw);
    const auto color = cv::Scalar(0, 0, 255);

    for (size_t i = 0; i != results.size(); ++i) {
        const auto& res = results.at(i);
        cv::rectangle(image_draw, res.box, color, 1);

        std::string flag = std::format("Cnt: {}, [{}, {}, {}, {}]", res.count, res.box.x, res.box.y, res.box.width, res.box.height);
        cv::putText(image_draw, flag, cv::Point(res.box.x, res.box.y - 5), cv::FONT_HERSHEY_PLAIN, 1.2, color, 1);
    }

    return image_draw;
}

void FeatureMatcher::add_results(ResultsVec results, int count)
{
    std::ranges::copy_if(results, std::back_inserter(filtered_results_), [&](const auto& res) { return res.count >= count; });

    merge_vector_(all_results_, std::move(results));
}

void FeatureMatcher::cherry_pick()
{
    sort_(all_results_);
    sort_(filtered_results_);

    if (auto index_opt = pythonic_index(filtered_results_.size(), param_.result_index)) {
        best_result_ = filtered_results_.at(*index_opt);
    }
}

void FeatureMatcher::sort_(ResultsVec& results) const
{
    switch (param_.order_by) {
    case ResultOrderBy::Horizontal:
        sort_by_horizontal_(results);
        break;
    case ResultOrderBy::Vertical:
        sort_by_vertical_(results);
        break;
    case ResultOrderBy::Score:
        sort_by_count_(results);
        break;
    case ResultOrderBy::Area:
        sort_by_area_(results);
        break;
    case ResultOrderBy::Random:
        sort_by_random_(results);
        break;
    default:
        LogError << "Not supported order by" << VAR(param_.order_by);
        break;
    }
}

MAA_VISION_NS_END
