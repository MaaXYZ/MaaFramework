#include "Comparator.h"

#include "Utils/Logger.hpp"
#include "Utils/NoWarningCV.h"
#include "VisionUtils.hpp"

MAA_VISION_NS_BEGIN

Comparator::Result Comparator::analyze(const cv::Mat& lhs, const cv::Mat& rhs) const
{
    if (lhs.size() != rhs.size()) {
        LogError << "lhs.size() != rhs.size()" << VAR(lhs) << VAR(rhs);
        return false;
    }

    if (param_.roi.empty()) {
        return comp(lhs, rhs, param_.method) > param_.threshold;
    }

    for (const cv::Rect& roi : param_.roi) {
        cv::Mat lhs_roi = lhs(correct_roi(roi, lhs));
        cv::Mat rhs_roi = rhs(correct_roi(roi, rhs));
        double similarity = comp(lhs_roi, rhs_roi, param_.method);

        LogTrace << VAR(roi) << VAR(similarity) << VAR(param_.threshold);

        if (similarity < param_.threshold) {
            return false;
        }
    }
    return true;
}

double Comparator::comp(const cv::Mat& lhs, const cv::Mat& rhs, int method)
{
    cv::Mat matched = match_template(lhs, rhs, method, false);
    if (matched.empty()) {
        return 0.0;
    }

    double min_val = 0.0, max_val = 0.0;
    cv::Point min_loc {}, max_loc {};
    cv::minMaxLoc(matched, &min_val, &max_val, &min_loc, &max_loc);

    if (std::isnan(max_val) || std::isinf(max_val)) {
        max_val = 0;
    }

    return max_val;
}

MAA_VISION_NS_END
