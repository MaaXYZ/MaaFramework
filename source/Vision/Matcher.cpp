#include "Matcher.h"

#include "MaaUtils/Logger.hpp"
#include "Resource/ResourceMgr.h"
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
    if (param_.templates.size() != param_.thresholds.size()) {
        LogError << "templates.size() != thresholds.size()" << VAR(param_.templates.size())
                 << VAR(param_.thresholds.size());
        return std::nullopt;
    }

    auto& templ_mgr = resource()->template_cfg();
    for (size_t i = 0; i != param_.templates.size(); ++i) {
        const std::string& name = param_.templates.at(i);
        const cv::Mat& templ = templ_mgr.get_template_image(name);
        if (templ.empty()) {
            LogWarn << "template is empty" << VAR(name) << VAR(i) << VAR(templ);
            continue;
        }
        double threshold = param_.thresholds.at(i);

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
    cv::Mat matched = match_template(image_with_roi(roi), templ, param_.method, param_.green_mask);
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
    return Result { .box = box, .score = max_val };
}

MAA_VISION_NS_END
