#include "Matcher.h"

#include "MaaUtils/Logger.hpp"
#include "Utils/NoWarningCV.h"
#include "Utils/StringMisc.hpp"
#include "VisionUtils.hpp"

MAA_VISION_NS_BEGIN

Matcher::ResultOpt Matcher::analyze() const
{
    if (!resource()) {
        LogError << "Resource not binded";
        return std::nullopt;
    }
    if (param_.template_images.empty()) {
        LogError << "templates is empty" << VAR(param_.template_paths);
        return std::nullopt;
    }

    if (param_.template_images.size() != param_.thresholds.size()) {
        LogError << "templates.size() != thresholds.size()" << VAR(param_.template_images.size())
                 << VAR(param_.thresholds.size());
        return std::nullopt;
    }

    for (size_t i = 0; i != param_.template_images.size(); ++i) {
        const cv::Mat& templ = param_.template_images.at(i);
        if (templ.empty()) {
            LogWarn << "template is empty" << VAR(param_.template_paths) << VAR(i) << VAR(templ);
            continue;
        }
        double threshold = param_.thresholds.at(i);

        LogTrace << param_.template_paths.at(i) << VAR(i) << VAR(threshold);
        auto ret = traverse_rois(templ, threshold);
        if (ret) {
            return ret;
        }
    }

    return std::nullopt;
}

Matcher::ResultOpt Matcher::traverse_rois(const cv::Mat& templ, double threshold) const
{
    if (!cache_.empty()) {
        return match_and_postproc(cache_, templ, threshold);
    }

    if (param_.roi.empty()) {
        return match_and_postproc(cv::Rect(0, 0, image_.cols, image_.rows), templ, threshold);
    }

    for (const cv::Rect& roi : param_.roi) {
        auto opt = match_and_postproc(roi, templ, threshold);
        if (opt) {
            return opt;
        }
    }

    return std::nullopt;
}

Matcher::ResultOpt Matcher::match_and_postproc(const cv::Rect& roi, const cv::Mat& templ, double threshold) const
{
    auto start = std::chrono::steady_clock::now();

    cv::Mat image = image_with_roi(roi);
    cv::Mat matched = match_template(image, templ, param_.method, param_.green_mask);
    if (matched.empty()) {
        return std::nullopt;
    }

    double min_val = 0.0, max_val = 0.0;
    cv::Point min_loc, max_loc;
    cv::minMaxLoc(matched, &min_val, &max_val, &min_loc, &max_loc);

    if (std::isnan(max_val) || std::isinf(max_val)) {
        max_val = 0;
    }

    if (max_val < threshold) {
        return std::nullopt;
    }

    cv::Rect box(max_loc.x + roi.x, max_loc.y + roi.y, templ.cols, templ.rows);

    if (max_val > threshold * 0.7) {
        auto costs = duration_since(start);
        LogTrace << VAR(box) << VAR(max_val) << VAR(costs);
    }

    return Result { .box = box, .score = max_val };
}

MAA_VISION_NS_END
