#include "MultiColorMatcher.h"

#include <format>

#include "MaaUtils/Logger.h"
#include "MaaUtils/NoWarningCV.hpp"
#include "VisionUtils.hpp"

MAA_VISION_NS_BEGIN

MultiColorMatcher::MultiColorMatcher(cv::Mat image, std::vector<cv::Rect> rois, MultiColorMatcherParam param, std::string name)
    : VisionBase(std::move(image), std::move(rois), std::move(name))
    , param_(std::move(param))
{
    analyze();
}

void MultiColorMatcher::analyze()
{
    auto start_time = std::chrono::steady_clock::now();

    while (next_roi()) {
        auto results = multi_color_match();
        add_results(std::move(results));
    }
    reset_roi();

    cherry_pick();

    auto cost = duration_since(start_time);
    LogDebug << name_ << VAR(all_results_) << VAR(filtered_results_) << VAR(best_result_) << VAR(cost)
             << VAR(param_.base_color) << VAR(param_.points.size()) << VAR(param_.degree) << VAR(param_.miss)
             << VAR(param_.count) << VAR(param_.orient);
}

cv::Scalar MultiColorMatcher::unpack_color(int color)
{
    return cv::Scalar(
        (color >> 16) & 0xFF, // R
        (color >> 8) & 0xFF,  // G
        color & 0xFF          // B
    );
}

cv::Scalar MultiColorMatcher::tolerance(int dev, int degree)
{
    if (dev != 0) {
        return unpack_color(dev);
    }
    int t = std::clamp(100 - degree, 0, 255);
    return cv::Scalar(t, t, t);
}

cv::Mat MultiColorMatcher::translate_mask(const cv::Mat& mask, int dx, int dy)
{
    // Goal: result(x, y) = mask(x + dx, y + dy)
    // i.e. translate the mask by (-dx, -dy)
    cv::Mat result = cv::Mat::zeros(mask.size(), CV_8U);

    const int w = mask.cols - std::abs(dx);
    const int h = mask.rows - std::abs(dy);
    if (w <= 0 || h <= 0) {
        return result;
    }

    const int sx = std::max(0, dx);       // source roi left-top x
    const int sy = std::max(0, dy);       // source roi left-top y
    const int dx_pos = std::max(0, -dx);  // dest roi left-top x
    const int dy_pos = std::max(0, -dy);  // dest roi left-top y

    mask(cv::Rect(sx, sy, w, h)).copyTo(result(cv::Rect(dx_pos, dy_pos, w, h)));
    return result;
}

MultiColorMatcher::ResultsVec MultiColorMatcher::multi_color_match() const
{
    cv::Mat image = image_with_roi();
    if (image.empty()) {
        return {};
    }

    cv::Mat rgb;
    cv::cvtColor(image, rgb, cv::COLOR_BGR2RGB);

    const auto base_tol = tolerance(param_.main, param_.degree);
    const auto base_c = unpack_color(param_.base_color);
    cv::Scalar base_lower = base_c - base_tol;
    cv::Scalar base_upper = base_c + base_tol;
    cv::Mat combined;
    cv::inRange(rgb, base_lower, base_upper, combined);

    const auto list_tol = tolerance(param_.list, param_.degree);
    for (const auto& p : param_.points) {
        const auto c = unpack_color(p.color);
        cv::Scalar lower = c - list_tol;
        cv::Scalar upper = c + list_tol;
        cv::Mat m;
        cv::inRange(rgb, lower, upper, m);
        // Translate so that result(x, y) reflects the match of image(x + p.dx, y + p.dy)
        cv::Mat shifted = translate_mask(m, p.dx, p.dy);
        cv::bitwise_and(combined, shifted, combined);
    }

    ResultsVec results;
    // Collect all non-zero points at once instead of per-pixel looping
    std::vector<cv::Point> locations;
    cv::findNonZero(combined, locations);
    results.reserve(locations.size());
    for (const auto& pt : locations) {
        cv::Rect box(roi_.x + pt.x, roi_.y + pt.y, 1, 1);
        results.emplace_back(MultiColorMatcherResult { .box = box, .count = 1 });
    }

    sort_by_orient(results, param_.orient);

    if (param_.miss > 0) {
        results = cluster_miss(std::move(results), param_.miss);
    }

    if (param_.count > 0 && static_cast<int>(results.size()) > param_.count) {
        results.resize(param_.count);
    }

    if (debug_draw_) {
        auto draw = draw_result(results);
        handle_draw(draw);
    }

    return results;
}

void MultiColorMatcher::sort_by_orient(ResultsVec& results, int orient)
{
    if (results.empty()) {
        return;
    }

    auto cx = std::ranges::max(results, {}, [](const auto& r) { return r.box.x; }).box.x;
    auto cy = std::ranges::max(results, {}, [](const auto& r) { return r.box.y; }).box.y;
    cx /= 2;
    cy /= 2;

    auto key = [&](const MultiColorMatcherResult& r) -> std::tuple<long long, long long> {
        int x = r.box.x;
        int y = r.box.y;
        switch (orient) {
        case 1: return { y, -x };
        case 2: return { -y, x };
        case 3: return { -y, -x };
        case 4: return { x, y };
        case 5: return { -x, y };
        case 6: return { x, -y };
        case 7: return { -x, -y };
        case 8: {
            long long dx = x - cx;
            long long dy = y - cy;
            return { dx * dx + dy * dy, x };
        }
        case 0:
        default: return { y, x };
        }
    };

    std::ranges::sort(results, [&](const auto& a, const auto& b) { return key(a) < key(b); });
}

MultiColorMatcher::ResultsVec MultiColorMatcher::cluster_miss(ResultsVec results, int miss)
{
    // Cluster by miss distance: take the first point, remove all points within miss range of it, repeat
    ResultsVec kept;
    kept.reserve(results.size());
    while (!results.empty()) {
        auto seed = results.front();
        kept.push_back(seed);
        std::erase_if(results, [&](const auto& r) {
            int dx = r.box.x - seed.box.x;
            int dy = r.box.y - seed.box.y;
            return std::abs(dx) <= miss && std::abs(dy) <= miss;
        });
    }
    return kept;
}

void MultiColorMatcher::add_results(ResultsVec results)
{
    merge_vector_(filtered_results_, results);
    merge_vector_(all_results_, std::move(results));
}

void MultiColorMatcher::cherry_pick()
{
    sort_(all_results_);
    sort_(filtered_results_);

    if (auto index_opt = pythonic_index(filtered_results_.size(), param_.result_index)) {
        best_result_ = filtered_results_.at(*index_opt);
    }
}

cv::Mat MultiColorMatcher::draw_result(const ResultsVec& results) const
{
    cv::Mat image_draw = draw_roi();
    const auto color_draw = cv::Scalar(0, 0, 255);

    for (size_t i = 0; i < results.size(); ++i) {
        const auto& res = results[i];
        cv::circle(image_draw, cv::Point(res.box.x, res.box.y), 2, color_draw, -1);

        std::string flag = std::format("{}: [{}, {}]", i, res.box.x, res.box.y);
        cv::putText(image_draw, flag, cv::Point(res.box.x + 4, res.box.y), cv::FONT_HERSHEY_PLAIN, 1.0, color_draw, 1);
        if (i > 50) {
            LogDebug << "too many results, skip drawing" << VAR(results.size());
            break;
        }
    }

    return image_draw;
}

void MultiColorMatcher::sort_(ResultsVec& results) const
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
