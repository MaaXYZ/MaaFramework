#include "TemplateComparator.h"

#include "MaaUtils/Logger.h"
#include "MaaUtils/NoWarningCV.hpp"
#include "VisionUtils.hpp"

MAA_VISION_NS_BEGIN

TemplateComparator::TemplateComparator(
    cv::Mat lhs,
    cv::Mat rhs,
    std::vector<cv::Rect> rois,
    TemplateComparatorParam param,
    std::string name)
    : VisionBase(std::move(lhs), std::move(rois), std::move(name))
    , rhs_image_(std::move(rhs))
    , param_(std::move(param))
    , low_score_better_(param_.method == cv::TemplateMatchModes::TM_SQDIFF || param_.method == cv::TemplateMatchModes::TM_SQDIFF_NORMED)
{
    analyze();
}

void TemplateComparator::analyze()
{
    if (image_.size() != rhs_image_.size()) {
        LogError << "lhs_image_.size() != rhs_image_.size()" << VAR(image_) << VAR(rhs_image_);
        return;
    }

    auto start_time = std::chrono::steady_clock::now();

    while (next_roi()) {
        cv::Mat lhs_roi = image_(correct_roi(roi_, image_));
        cv::Mat rhs_roi = rhs_image_(correct_roi(roi_, rhs_image_));
        double score = comp(lhs_roi, rhs_roi, param_.method);
        Result res = Result { .box = roi_, .score = score };
        add_results({ std::move(res) }, param_.threshold);
    }

    cherry_pick();
    auto cost = duration_since(start_time);

    LogDebug << name_ << VAR(all_results_) << VAR(filtered_results_) << VAR(best_result_) << VAR(cost) << VAR(param_.threshold)
             << VAR(param_.method);
}

void TemplateComparator::add_results(ResultsVec results, double threshold)
{
    std::ranges::copy_if(results, std::back_inserter(filtered_results_), [&](const auto& res) { return comp_score(threshold, res.score); });

    merge_vector_(all_results_, std::move(results));
}

void TemplateComparator::cherry_pick()
{
    sort_by_score_(all_results_, low_score_better_);
    sort_by_score_(filtered_results_, low_score_better_);

    if (!filtered_results_.empty()) {
        best_result_ = filtered_results_.front();
    }
}

double TemplateComparator::comp(const cv::Mat& lhs, const cv::Mat& rhs, int method)
{
    bool invert_score = false;
    if (method >= TemplateMatcherParam::kMethodInvertBase) {
        invert_score = true;
        method -= TemplateMatcherParam::kMethodInvertBase;
    }

    cv::Mat matched;
    cv::matchTemplate(lhs, rhs, matched, method);

    if (invert_score) {
        matched = 1.0f - matched;
    }

    double min_val = 0.0, max_val = 0.0;
    cv::Point min_loc {}, max_loc {};
    cv::minMaxLoc(matched, &min_val, &max_val, &min_loc, &max_loc);

    double val = low_score_better_ ? min_val : max_val;

    if (std::isnan(val) || std::isinf(val)) {
        val = low_score_better_ ? std::numeric_limits<double>::max() : 0;
    }

    return val;
}

bool TemplateComparator::comp_score(double s1, double s2) const
{
    return low_score_better_ ? s1 > s2 : s1 < s2;
}

MAA_VISION_NS_END
