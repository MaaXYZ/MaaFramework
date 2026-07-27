#include "DetectionPostProcessor.h"

#include <algorithm>
#include <cmath>
#include <limits>

MAA_VISION_NS_BEGIN

namespace NeuralNetwork
{

namespace
{

cv::Rect truncate_box(const cv::Rect2f& box)
{
    return {
        static_cast<int>(box.x),
        static_cast<int>(box.y),
        static_cast<int>(box.width),
        static_cast<int>(box.height),
    };
}

int64_t area(const cv::Rect& box)
{
    return static_cast<int64_t>(box.width) * box.height;
}

int64_t intersection_area(const cv::Rect2f& lhs, const cv::Rect2f& rhs)
{
    return area(truncate_box(lhs) & truncate_box(rhs));
}

float iou(const cv::Rect2f& lhs, const cv::Rect2f& rhs)
{
    const cv::Rect lhs_box = truncate_box(lhs);
    const cv::Rect rhs_box = truncate_box(rhs);
    const int64_t intersection = area(lhs_box & rhs_box);
    const int64_t union_area = area(lhs_box) + area(rhs_box) - intersection;
    return union_area > 0 ? static_cast<float>(intersection) / static_cast<float>(union_area) : 0.0F;
}

} // namespace

bool DetectionPostProcessor::sanitize(std::vector<DetectionCandidate>& candidates, cv::Size roi_size, std::string& error)
{
    if (roi_size.width <= 0 || roi_size.height <= 0) {
        error = "ROI size must be positive";
        return false;
    }

    std::vector<DetectionCandidate> output;
    output.reserve(candidates.size());
    for (auto candidate : candidates) {
        const auto& box = candidate.box;
        if (!std::isfinite(box.x) || !std::isfinite(box.y) || !std::isfinite(box.width) || !std::isfinite(box.height)
            || !std::isfinite(candidate.score)) {
            error = "detection candidate contains a non-finite value";
            return false;
        }
        if (candidate.class_id < 0) {
            error = "detection candidate contains a negative class id";
            return false;
        }
        if (box.width < 0.0F || box.height < 0.0F) {
            error = "detection candidate contains a negative box size";
            return false;
        }

        constexpr float kScoreTolerance = 1e-5F;
        if (candidate.score < -kScoreTolerance || candidate.score > 1.0F + kScoreTolerance) {
            error = "detection candidate score is outside [0, 1]";
            return false;
        }
        candidate.score = std::clamp(candidate.score, 0.0F, 1.0F);

        constexpr double kIntMin = std::numeric_limits<int>::min();
        constexpr double kIntMax = std::numeric_limits<int>::max();
        if (box.x < kIntMin || box.x > kIntMax || box.y < kIntMin || box.y > kIntMax || box.width > kIntMax || box.height > kIntMax) {
            error = "detection candidate box exceeds the supported integer range";
            return false;
        }

        const cv::Rect integer_box = to_integer_box(box);
        const int64_t left = std::clamp<int64_t>(integer_box.x, 0, roi_size.width);
        const int64_t top = std::clamp<int64_t>(integer_box.y, 0, roi_size.height);
        const int64_t right = std::clamp<int64_t>(static_cast<int64_t>(integer_box.x) + integer_box.width, 0, roi_size.width);
        const int64_t bottom = std::clamp<int64_t>(static_cast<int64_t>(integer_box.y) + integer_box.height, 0, roi_size.height);
        if (right <= left || bottom <= top) {
            continue;
        }

        candidate.box = {
            static_cast<float>(left),
            static_cast<float>(top),
            static_cast<float>(right - left),
            static_cast<float>(bottom - top),
        };
        output.emplace_back(std::move(candidate));
    }

    candidates = std::move(output);
    error.clear();
    return true;
}

bool DetectionPostProcessor::build_threshold_policy(
    const std::vector<int>& expected,
    const std::vector<double>& thresholds,
    double default_threshold,
    DetectionThresholdPolicy& policy,
    std::string& error)
{
    policy = { };
    if (expected.empty()) {
        policy.score_floor = thresholds.empty() ? default_threshold : thresholds.front();
        error.clear();
        return true;
    }
    if (expected.size() != thresholds.size()) {
        error = "expected and threshold counts do not match";
        return false;
    }

    policy.expected_thresholds.reserve(expected.size());
    for (size_t index = 0; index < expected.size(); ++index) {
        const int class_id = expected[index];
        const auto duplicate = std::ranges::find(policy.expected_thresholds, class_id, &ExpectedThreshold::class_id);
        if (duplicate != policy.expected_thresholds.end()) {
            continue;
        }

        policy.expected_thresholds.emplace_back(
            ExpectedThreshold {
                .class_id = class_id,
                .threshold = thresholds[index],
            });
    }
    policy.score_floor = std::ranges::min(policy.expected_thresholds, { }, &ExpectedThreshold::threshold).threshold;
    error.clear();
    return true;
}

void DetectionPostProcessor::filter_by_score(std::vector<DetectionCandidate>& candidates, double score_floor)
{
    std::erase_if(candidates, [score_floor](const auto& candidate) { return candidate.score < score_floor; });
}

cv::Rect DetectionPostProcessor::to_integer_box(const cv::Rect2f& box)
{
    return truncate_box(box);
}

std::vector<DetectionCandidate> DetectionPostProcessor::nms(std::vector<DetectionCandidate> candidates, NmsPolicy policy, float threshold)
{
    std::ranges::stable_sort(candidates, std::greater { }, &DetectionCandidate::score);
    if (policy == NmsPolicy::None) {
        return candidates;
    }

    std::vector<DetectionCandidate> output;
    std::vector<bool> suppressed(candidates.size(), false);
    for (size_t i = 0; i < candidates.size(); ++i) {
        if (suppressed[i]) {
            continue;
        }

        output.emplace_back(candidates[i]);
        for (size_t j = i + 1; j < candidates.size(); ++j) {
            if (suppressed[j]) {
                continue;
            }

            const auto& selected = candidates[i];
            const auto& candidate = candidates[j];
            if (policy == NmsPolicy::ClassAwareIoU) {
                if (selected.class_id == candidate.class_id && iou(selected.box, candidate.box) >= threshold) {
                    suppressed[j] = true;
                }
            }
            else {
                const int64_t candidate_area = area(truncate_box(candidate.box));
                if (candidate_area > 0
                    && static_cast<double>(intersection_area(selected.box, candidate.box)) >= threshold * candidate_area) {
                    suppressed[j] = true;
                }
            }
        }
    }

    return output;
}

} // namespace NeuralNetwork

MAA_VISION_NS_END
