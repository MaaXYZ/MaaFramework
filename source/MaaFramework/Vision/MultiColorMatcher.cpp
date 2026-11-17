#include "MultiColorMatcher.h"

#include <format>
#include <unordered_map>
#include <unordered_set>

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

    ResultsVec raw_results;
    while (next_roi()) {
        auto results = multi_color_match();
        merge_vector_(raw_results, std::move(results));
    }
    reset_roi();

    // Overlapping ROIs produce identical coordinates; dedupe them before the
    // clustering / count steps so duplicates don't consume the count limit
    raw_results = dedupe_hits(std::move(raw_results));

    if (param_.miss > 0) {
        // orient decides which hit survives clustering, so it only takes part
        // in the selection when clustering is enabled
        sort_by_orient(raw_results, param_.orient);
        raw_results = cluster_miss(raw_results, param_.miss, param_.count);
    }
    else {
        // Without clustering, sort into the documented result order before
        // applying the count limit
        sort_(raw_results);
    }

    if (param_.count > 0 && static_cast<int>(raw_results.size()) > param_.count) {
        raw_results.resize(param_.count);
    }

    add_results(std::move(raw_results));

    cherry_pick();

    if (debug_draw_) {
        handle_draw(draw_result(filtered_results_));
    }

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

cv::Mat MultiColorMatcher::translate_mask(const cv::Mat& mask, long long dx, long long dy)
{
    // Goal: result(x, y) = mask(x + dx, y + dy)
    // i.e. translate the mask by (-dx, -dy)
    // Offsets are computed in 64-bit so extreme pipeline values (e.g. INT_MIN)
    // can neither overflow nor trigger UB
    cv::Mat result = cv::Mat::zeros(mask.size(), CV_8U);

    const long long w = static_cast<long long>(mask.cols) - std::llabs(dx);
    const long long h = static_cast<long long>(mask.rows) - std::llabs(dy);
    if (w <= 0 || h <= 0) {
        return result;
    }

    const long long sx = std::max<long long>(0, dx);       // source roi left-top x
    const long long sy = std::max<long long>(0, dy);       // source roi left-top y
    const long long tx = std::max<long long>(0, -dx);      // dest roi left-top x
    const long long ty = std::max<long long>(0, -dy);      // dest roi left-top y

    mask(cv::Rect(static_cast<int>(sx), static_cast<int>(sy), static_cast<int>(w), static_cast<int>(h)))
        .copyTo(result(cv::Rect(static_cast<int>(tx), static_cast<int>(ty), static_cast<int>(w), static_cast<int>(h))));
    return result;
}

MultiColorMatcher::ResultsVec MultiColorMatcher::multi_color_match() const
{
    // Offset points may land outside the ROI, so sample an expanded region
    // (clamped to the image) instead of the bare ROI crop; otherwise a valid
    // base hit near the ROI edge would be dropped because its offset point
    // falls into the zero-filled area outside the crop
    long long min_dx = 0, max_dx = 0, min_dy = 0, max_dy = 0;
    for (const auto& p : param_.points) {
        min_dx = std::min(min_dx, static_cast<long long>(p.dx));
        max_dx = std::max(max_dx, static_cast<long long>(p.dx));
        min_dy = std::min(min_dy, static_cast<long long>(p.dy));
        max_dy = std::max(max_dy, static_cast<long long>(p.dy));
    }

    const long long sample_x1 = std::clamp<long long>(roi_.x - std::max(0LL, -min_dx), 0, image_.cols);
    const long long sample_y1 = std::clamp<long long>(roi_.y - std::max(0LL, -min_dy), 0, image_.rows);
    const long long sample_x2 =
        std::clamp<long long>(static_cast<long long>(roi_.x) + roi_.width + std::max(0LL, max_dx), 0, image_.cols);
    const long long sample_y2 =
        std::clamp<long long>(static_cast<long long>(roi_.y) + roi_.height + std::max(0LL, max_dy), 0, image_.rows);
    const cv::Rect sample(
        static_cast<int>(sample_x1),
        static_cast<int>(sample_y1),
        static_cast<int>(sample_x2 - sample_x1),
        static_cast<int>(sample_y2 - sample_y1));
    if (sample.empty()) {
        return {};
    }

    // Base point candidates are restricted to the configured ROI
    const cv::Rect roi_local(roi_.x - sample.x, roi_.y - sample.y, roi_.width, roi_.height);

    cv::Mat rgb;
    cv::cvtColor(image_(sample), rgb, cv::COLOR_BGR2RGB);

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

    // Collect all non-zero points at once instead of per-pixel looping; the
    // vector is bounded by the sample area, and the orient / clustering / count
    // selection below must see every candidate, so no truncation here
    std::vector<cv::Point> locations;
    cv::findNonZero(combined, locations);

    ResultsVec results;
    results.reserve(locations.size());
    for (const auto& pt : locations) {
        if (!roi_local.contains(pt)) {
            continue;
        }
        cv::Rect box(sample.x + pt.x, sample.y + pt.y, 1, 1);
        results.emplace_back(MultiColorMatcherResult { .box = box, .count = 1 });
    }

    return results;
}

MultiColorMatcher::ResultsVec MultiColorMatcher::dedupe_hits(ResultsVec results)
{
    if (results.empty()) {
        return results;
    }

    // Coordinates are image positions, so both fit in unsigned 32-bit halves
    std::unordered_set<long long> seen;
    seen.reserve(results.size());
    ResultsVec kept;
    kept.reserve(results.size());
    for (const auto& r : results) {
        const long long key = (static_cast<long long>(static_cast<unsigned int>(r.box.x)) << 32)
                            | static_cast<unsigned int>(r.box.y);
        if (seen.insert(key).second) {
            kept.push_back(r);
        }
    }
    return kept;
}

void MultiColorMatcher::sort_by_orient(ResultsVec& results, int orient)
{
    if (results.empty()) {
        return;
    }

    // orient 8 sorts by distance to the center; derive the center from the hit
    // bounds, not from half the max coordinates (which breaks when hits are
    // offset from the origin)
    const auto [min_x, max_x] = std::ranges::minmax(results, {}, [](const auto& r) { return r.box.x; });
    const auto [min_y, max_y] = std::ranges::minmax(results, {}, [](const auto& r) { return r.box.y; });
    const long long cx = (static_cast<long long>(min_x.box.x) + max_x.box.x) / 2;
    const long long cy = (static_cast<long long>(min_y.box.y) + max_y.box.y) / 2;

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

MultiColorMatcher::ResultsVec MultiColorMatcher::cluster_miss(const ResultsVec& results, int miss, int max_keep)
{
    // Grid-based clustering, ~O(n): iterate in order, keep a point only if no
    // previously kept point lies within Chebyshev distance `miss`
    ResultsVec kept;
    kept.reserve(results.size());

    const long long cell = static_cast<long long>(miss) + 1;
    // Encode the grid cell coordinates into a 64-bit key; going through the
    // unsigned halves avoids the UB of left-shifting negative signed values
    // when hits sit near the top / left edge
    auto cell_key = [](long long cx, long long cy) {
        return (static_cast<unsigned long long>(static_cast<unsigned int>(cx)) << 32)
             | static_cast<unsigned long long>(static_cast<unsigned int>(cy));
    };
    std::unordered_map<long long, std::vector<cv::Point>> grid;

    for (const auto& r : results) {
        const long long gcx = r.box.x / cell;
        const long long gcy = r.box.y / cell;

        // Cell size is miss + 1, so any blocking point must lie in the 3x3
        // neighborhood of cells
        bool blocked = false;
        for (long long gx = gcx - 1; gx <= gcx + 1 && !blocked; ++gx) {
            for (long long gy = gcy - 1; gy <= gcy + 1 && !blocked; ++gy) {
                auto it = grid.find(cell_key(gx, gy));
                if (it == grid.end()) {
                    continue;
                }
                for (const auto& p : it->second) {
                    if (std::abs(p.x - r.box.x) <= miss && std::abs(p.y - r.box.y) <= miss) {
                        blocked = true;
                        break;
                    }
                }
            }
        }
        if (blocked) {
            continue;
        }

        kept.push_back(r);
        grid[cell_key(gcx, gcy)].emplace_back(r.box.x, r.box.y);

        // Once the count limit is reached, every later candidate is either
        // removed by clustering or truncated by count, so stop here; this
        // bounds the work without dropping any would-be-selected hit
        if (max_keep > 0 && static_cast<int>(kept.size()) >= max_keep) {
            break;
        }
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
