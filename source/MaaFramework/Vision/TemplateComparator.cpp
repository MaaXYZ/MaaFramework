#include "TemplateComparator.h"

#include "Utils/Logger.h"
#include "Utils/NoWarningCV.hpp"
#include "VisionUtils.hpp"

MAA_VISION_NS_BEGIN

TemplateComparator::ResultsVec TemplateComparator::analyze(const cv::Mat& lhs, const cv::Mat& rhs) const
{
    if (lhs.size() != rhs.size()) {
        LogError << "lhs.size() != rhs.size()" << VAR(lhs) << VAR(rhs);
        return {};
    }

    auto start_time = std::chrono::steady_clock::now();

    ResultsVec results = foreach_rois(lhs, rhs);

    auto cost = duration_since(start_time);
    LogDebug << "Raw:" << VAR(results) << VAR(cost);

    double threshold = param_.threshold;
    filter(results, threshold);

    cost = duration_since(start_time);
    LogDebug << "Proc:" << VAR(results) << VAR(threshold) << VAR(cost);
    return results;
}

TemplateComparator::ResultsVec TemplateComparator::foreach_rois(const cv::Mat& lhs, const cv::Mat& rhs) const
{
    auto method = param_.method;

    if (param_.roi.empty()) {
        double score = comp(lhs, rhs, method);
        return { Result { .box = cv::Rect(0, 0, lhs.cols, lhs.rows), .score = score } };
    }

    ResultsVec results;
    for (const cv::Rect& roi : param_.roi) {
        cv::Mat lhs_roi = lhs(correct_roi(roi, lhs));
        cv::Mat rhs_roi = rhs(correct_roi(roi, rhs));

        double score = comp(lhs_roi, rhs_roi, method);
        Result res { .box = roi, .score = score };
        results.emplace_back(std::move(res));
    }

    return results;
}

void TemplateComparator::filter(ResultsVec& results, double threshold) const
{
    auto remove_iter =
        std::remove_if(results.begin(), results.end(), [threshold](const auto& res) { return res.score < threshold; });
    results.erase(remove_iter, results.end());
}

double TemplateComparator::comp(const cv::Mat& lhs, const cv::Mat& rhs, int method)
{
    cv::Mat matched;
    cv::matchTemplate(lhs, rhs, matched, method);

    double min_val = 0.0, max_val = 0.0;
    cv::Point min_loc {}, max_loc {};
    cv::minMaxLoc(matched, &min_val, &max_val, &min_loc, &max_loc);

    if (std::isnan(max_val) || std::isinf(max_val)) {
        max_val = 0;
    }

    return max_val;
}

MAA_VISION_NS_END
