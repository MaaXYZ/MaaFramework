#pragma once

#include <functional>
#include <string>
#include <vector>

#include "Common/Conf.h"
#include "MaaUtils/NoWarningCV.hpp"

MAA_VISION_NS_BEGIN

namespace NeuralNetwork
{

enum class NmsPolicy
{
    None,
    ClassAwareIoU,
    CandidateCoverage,
};

struct DetectionCandidate
{
    cv::Rect2f box;
    float score = 0.0F;
    int class_id = -1;
};

struct ExpectedThreshold
{
    int class_id = -1;
    double threshold = 0.0;
};

struct DetectionThresholdPolicy
{
    double score_floor = 0.0;
    std::vector<ExpectedThreshold> expected_thresholds;
};

class DetectionPostProcessor
{
public:
    static bool sanitize(std::vector<DetectionCandidate>& candidates, cv::Size roi_size, std::string& error);
    static bool build_threshold_policy(
        const std::vector<int>& expected,
        const std::vector<double>& thresholds,
        double default_threshold,
        DetectionThresholdPolicy& policy,
        std::string& error);
    static void filter_by_score(std::vector<DetectionCandidate>& candidates, double score_floor);
    static cv::Rect to_integer_box(const cv::Rect2f& box);

    static std::vector<DetectionCandidate> nms(std::vector<DetectionCandidate> candidates, NmsPolicy policy, float threshold);

    template <typename Result, typename ClassProjection, typename ScoreProjection>
    static std::vector<Result> select_best_per_expected(
        const std::vector<Result>& results,
        const std::vector<ExpectedThreshold>& expected_thresholds,
        ClassProjection class_projection,
        ScoreProjection score_projection)
    {
        if (expected_thresholds.empty()) {
            return results;
        }

        std::vector<Result> output;
        output.reserve(expected_thresholds.size());
        for (const auto& expected : expected_thresholds) {
            const Result* best = nullptr;
            for (const auto& result : results) {
                if (std::invoke(class_projection, result) != expected.class_id
                    || std::invoke(score_projection, result) < expected.threshold) {
                    continue;
                }
                if (!best || std::invoke(score_projection, result) > std::invoke(score_projection, *best)) {
                    best = &result;
                }
            }
            if (best) {
                output.emplace_back(*best);
            }
        }
        return output;
    }
};

} // namespace NeuralNetwork

MAA_VISION_NS_END
