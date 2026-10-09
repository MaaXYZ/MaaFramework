#include "TemplateComparator.h"

#include "MaaUtils/Logger.h"
#include "MaaUtils/NoWarningCV.hpp"
#include "VisionUtils.hpp"

MAA_VISION_NS_BEGIN

namespace
{

struct PatchStats
{
    bool zero_variance = false; // 每个通道 min == max，即整块是同一颜色
    bool all_zero = false;      // 每个通道 min == max == 0，即整块全 0
    bool near_constant = false; // 每个通道 max - min <= 1，几乎无纹理：分数不可靠，值得记一条日志
};

PatchStats stats_of(const cv::Mat& patch)
{
    PatchStats stats { .zero_variance = true, .all_zero = true, .near_constant = true };

    for (int channel = 0; channel < patch.channels(); ++channel) {
        cv::Mat single;
        if (patch.channels() == 1) {
            single = patch;
        }
        else {
            cv::extractChannel(patch, single, channel);
        }

        // 用 min == max 判断常量：这是逐像素的精确比较，不依赖 meanStdDev 的浮点残差
        // （ARM 上常量 patch 也可能算出非 0 的 stddev，那样会漏掉退化分支）
        double min_val = 0.0, max_val = 0.0;
        cv::minMaxLoc(single, &min_val, &max_val);

        stats.zero_variance = stats.zero_variance && min_val == max_val;
        stats.all_zero = stats.all_zero && min_val == 0.0 && max_val == 0.0;
        stats.near_constant = stats.near_constant && max_val - min_val <= 1.0;
    }

    return stats;
}

// 归一化方法的分母在某些输入下恒为 0，此时 cv::matchTemplate 返回的是与相似度无关的硬编码值
// （CCOEFF_NORMED 走模板常量的提前返回得 1，其余走 "avoid rounding errors" 分支得 0），
// 调用方（如 wait_freezes）会把它当成"变化极大"，见 OpenCV common_matchTemplate。
bool is_undefined_score(const PatchStats& lhs_stats, const PatchStats& rhs_stats, int method)
{
    switch (method) {
    case cv::TemplateMatchModes::TM_SQDIFF_NORMED:
    case cv::TemplateMatchModes::TM_CCORR_NORMED:
        // 分母为 sqrt(ΣT²·ΣI²)，任一侧全 0 即为 0
        return lhs_stats.all_zero || rhs_stats.all_zero;
    case cv::TemplateMatchModes::TM_CCOEFF_NORMED:
        // 分母为 sqrt(Σ(T-T̄)²·Σ(I-Ī)²)，任一侧无方差即为 0
        return lhs_stats.zero_variance || rhs_stats.zero_variance;
    default:
        return false;
    }
}

bool is_comparable(const cv::Mat& lhs, const cv::Mat& rhs)
{
    return !lhs.empty() && !rhs.empty() && lhs.size() == rhs.size() && lhs.type() == rhs.type();
}

} // namespace

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
        cv::Mat lhs_roi = image_(roi_);
        cv::Mat rhs_roi = rhs_image_(roi_);
        double score = comp(lhs_roi, rhs_roi, param_.method);
        Result res = Result { .box = roi_, .score = score };
        add_results({ std::move(res) }, param_.threshold);

        if (debug_draw_) {
            auto draw = draw_result(roi_, score);
            handle_draw(draw);
        }
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

    // 尺寸/类型不一致时保持原路径，交给 matchTemplate 自己报错
    if (is_comparable(lhs, rhs)) {
        const PatchStats lhs_stats = stats_of(lhs);
        const PatchStats rhs_stats = stats_of(rhs);

        if (is_undefined_score(lhs_stats, rhs_stats, method)) {
            return degenerate_score(lhs, rhs, method, invert_score);
        }

        // 近常量 patch 上归一化分数没有区分度（见 PR 说明），这里留一条线索便于线上定位
        if (lhs_stats.near_constant || rhs_stats.near_constant) {
            LogDebug << name_ << "near-constant patch, score may be unreliable" << VAR(method) << VAR(lhs.size()) << VAR(rhs.size());
        }
    }

    cv::Mat matched;
    cv::matchTemplate(lhs, rhs, matched, method);

    if (invert_score) {
        matched = 1.0f - matched;
    }

    double min_val = 0.0, max_val = 0.0;
    cv::Point min_loc { }, max_loc { };
    cv::minMaxLoc(matched, &min_val, &max_val, &min_loc, &max_loc);

    double val = low_score_better_ ? min_val : max_val;

    if (std::isnan(val) || std::isinf(val)) {
        val = low_score_better_ ? std::numeric_limits<double>::max() : 0;
    }

    return val;
}

// 退化输入下 matchTemplate 的分母为 0，没有可用的相似度，这里直接用像素本身是否一致来定分：
// 一致即为最优（wait_freezes 判定为静止），不一致则沿用原先兜底的最差取值。
double TemplateComparator::degenerate_score(const cv::Mat& lhs, const cv::Mat& rhs, int method, bool invert_score) const
{
    // 方向必须按去掉 kMethodInvertBase 后的真实 cv method 判断：SQDIFF 系"越小越接近"。
    // low_score_better_ 取自未去偏移的 param_.method，带 10000 时恒为 false，不能用在这里。
    const bool low_score_better = (method == cv::TemplateMatchModes::TM_SQDIFF || method == cv::TemplateMatchModes::TM_SQDIFF_NORMED);

    const double max_diff = cv::norm(lhs, rhs, cv::NORM_INF);
    const bool identical = max_diff == 0.0;

    // 退化分支的取值只由像素本身决定，打出 max_diff 便于线上区分"两帧一致"与"两侧确实不同"
    LogDebug << name_ << "degenerate patch" << VAR(max_diff) << VAR(lhs.size()) << VAR(rhs.size()) << VAR(method) << VAR(invert_score)
             << VAR(low_score_better);

    // 非反转沿用原兜底：SQDIFF 系最差为 DBL_MAX、其余为 0（意在"比任何真实分数都差"）。
    // 反转后分数会被 1 - x 映射，SQDIFF 系最差取归一化上界 1（映射为 0），避免产出 -DBL_MAX 这类越界值。
    const double worst = low_score_better ? (invert_score ? 1.0 : std::numeric_limits<double>::max()) : 0.0;
    const double val = identical ? (low_score_better ? 0.0 : 1.0) : worst;

    return invert_score ? 1.0 - val : val;
}

bool TemplateComparator::comp_score(double s1, double s2) const
{
    return low_score_better_ ? s1 > s2 : s1 < s2;
}

cv::Mat TemplateComparator::draw_result(const cv::Rect& roi, double score) const
{
    int width = image_.cols + rhs_image_.cols;
    int height = std::max(image_.rows, rhs_image_.rows);
    cv::Mat draw = cv::Mat::zeros(height, width, image_.type());

    image_.copyTo(draw(cv::Rect(0, 0, image_.cols, image_.rows)));
    rhs_image_.copyTo(draw(cv::Rect(image_.cols, 0, rhs_image_.cols, rhs_image_.rows)));

    const cv::Scalar roi_color(0, 255, 0);
    const cv::Scalar score_color(0, 0, 255);

    cv::putText(draw, name_, cv::Point(5, image_.rows - 5), cv::FONT_HERSHEY_SIMPLEX, 1, roi_color, 2);

    cv::rectangle(draw, roi, roi_color, 1);
    std::string roi_flag = std::format("ROI: [{}, {}, {}, {}]", roi.x, roi.y, roi.width, roi.height);
    cv::putText(draw, roi_flag, cv::Point(roi.x, roi.y - 5), cv::FONT_HERSHEY_PLAIN, 1.2, roi_color, 1);

    cv::Rect rhs_roi(roi.x + image_.cols, roi.y, roi.width, roi.height);
    cv::rectangle(draw, rhs_roi, roi_color, 1);
    cv::putText(draw, roi_flag, cv::Point(rhs_roi.x, rhs_roi.y - 5), cv::FONT_HERSHEY_PLAIN, 1.2, roi_color, 1);

    cv::line(draw, roi.tl(), rhs_roi.tl(), roi_color, 1);

    std::string score_flag = std::format("Score: {:.3f}", score);
    cv::putText(draw, score_flag, cv::Point(roi.x, roi.y + roi.height + 30), cv::FONT_HERSHEY_SIMPLEX, 1, score_color, 2);

    return draw;
}

MAA_VISION_NS_END
