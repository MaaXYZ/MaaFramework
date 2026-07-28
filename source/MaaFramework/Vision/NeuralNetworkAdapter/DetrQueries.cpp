#include "NeuralNetworkAdapter.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>

MAA_VISION_NS_BEGIN

namespace NeuralNetworkAdapter
{

namespace
{

float sigmoid(float value)
{
    return value >= 0.0F ? 1.0F / (1.0F + std::exp(-value)) : std::exp(value) / (1.0F + std::exp(value));
}

struct ScoredIndex
{
    float score = 0.0F;
    size_t flat_index = 0;
};

class DetrQueriesAdapter final : public IAdapter
{
public:
    explicit DetrQueriesAdapter(const AdapterOptions& options)
        : activation_(options.detr_activation)
        , top_k_(options.detr_top_k)
    {
    }

    bool validate(std::span<const TensorSpec> tensors, std::string& error) const override
    {
        if (tensors.size() != 2 || tensors[0].element_type != TensorElementType::Float32
            || tensors[1].element_type != TensorElementType::Float32) {
            error = "DetrQueries requires FP32 boxes and logits outputs";
            return false;
        }
        const auto& boxes = tensors[0].shape;
        const auto& logits = tensors[1].shape;
        const int64_t minimum_logits = activation_ == DetrActivation::SoftmaxWithNoObject ? 2 : 1;
        if (boxes.size() != 3 || !has_batch_one(boxes) || boxes[1] == 0 || boxes[1] < -1 || boxes[2] != 4) {
            error = "DetrQueries boxes output must have shape [1, Q, 4]";
            return false;
        }
        if (logits.size() != 3 || !has_batch_one(logits) || logits[1] == 0 || logits[1] < -1
            || (logits[2] != -1 && logits[2] < minimum_logits)) {
            error = "DetrQueries logits output has an invalid shape";
            return false;
        }
        if (boxes[1] > 0 && logits[1] > 0 && boxes[1] != logits[1]) {
            error = "DetrQueries boxes and logits query counts differ";
            return false;
        }
        if (top_k_ == 0) {
            error = "DetrQueries top_k must be positive";
            return false;
        }
        error.clear();
        return true;
    }

    bool decode(std::span<const TensorView> tensors, const DecodeOptions& options, DecodedDetections& output, std::string& error)
        const override
    {
        if (tensors.size() != 2 || tensors[0].element_type != TensorElementType::Float32
            || tensors[1].element_type != TensorElementType::Float32) {
            error = "DetrQueries requires FP32 boxes and logits outputs";
            return false;
        }
        const auto& boxes = tensors[0];
        const auto& logits = tensors[1];
        const int64_t minimum_logits = activation_ == DetrActivation::SoftmaxWithNoObject ? 2 : 1;
        if (boxes.shape.size() != 3 || boxes.shape[0] != 1 || boxes.shape[1] < 0 || boxes.shape[2] != 4 || logits.shape.size() != 3
            || logits.shape[0] != 1 || logits.shape[1] != boxes.shape[1] || logits.shape[2] < minimum_logits) {
            error = "DetrQueries outputs have invalid runtime shapes";
            return false;
        }

        const size_t query_count = static_cast<size_t>(boxes.shape[1]);
        const size_t logits_count = static_cast<size_t>(logits.shape[2]);
        const size_t class_count = activation_ == DetrActivation::SoftmaxWithNoObject ? logits_count - 1 : logits_count;
        if (class_count > static_cast<size_t>(std::numeric_limits<int>::max()) || boxes.float_data.size() != query_count * 4
            || logits.float_data.size() != query_count * logits_count) {
            error = "DetrQueries outputs have invalid dimensions";
            return false;
        }
        if (!check_finite(boxes.float_data, "DetrQueries", error) || !check_finite(logits.float_data, "DetrQueries", error)) {
            return false;
        }

        std::vector<ScoredIndex> scored;
        scored.reserve(query_count * class_count);
        for (size_t query = 0; query < query_count; ++query) {
            const auto query_logits = logits.float_data.subspan(query * logits_count, logits_count);
            if (activation_ == DetrActivation::Sigmoid) {
                for (size_t class_id = 0; class_id < class_count; ++class_id) {
                    scored.emplace_back(
                        ScoredIndex { .score = sigmoid(query_logits[class_id]), .flat_index = query * class_count + class_id });
                }
                continue;
            }

            const float maximum = *std::ranges::max_element(query_logits);
            float denominator = 0.0F;
            for (float value : query_logits) {
                denominator += std::exp(value - maximum);
            }
            if (!std::isfinite(denominator) || denominator <= 0.0F) {
                error = "DetrQueries softmax denominator is invalid";
                return false;
            }
            for (size_t class_id = 0; class_id < class_count; ++class_id) {
                scored.emplace_back(
                    ScoredIndex {
                        .score = std::exp(query_logits[class_id] - maximum) / denominator,
                        .flat_index = query * class_count + class_id,
                    });
            }
        }

        const size_t selected_count = std::min(top_k_, scored.size());
        std::ranges::partial_sort(
            scored,
            scored.begin() + static_cast<std::ptrdiff_t>(selected_count),
            [](const ScoredIndex& lhs, const ScoredIndex& rhs) {
                return lhs.score != rhs.score ? lhs.score > rhs.score : lhs.flat_index < rhs.flat_index;
            });
        scored.resize(selected_count);

        output = { };
        output.class_count = class_count;
        output.candidates.reserve(selected_count);
        for (const auto& selected : scored) {
            if (selected.score < options.score_floor) {
                continue;
            }
            const size_t query = selected.flat_index / class_count;
            const size_t class_id = selected.flat_index % class_count;
            const auto box = boxes.float_data.subspan(query * 4, 4);
            output.candidates.emplace_back(
                RawDetection {
                    .box = { box[0], box[1], box[2], box[3] },
                    .score = selected.score,
                    .class_id = static_cast<int>(class_id),
                });
        }
        error.clear();
        return true;
    }

private:
    DetrActivation activation_;
    size_t top_k_ = 300;
};

} // namespace

std::unique_ptr<IAdapter> create_detr_queries(const AdapterOptions& options)
{
    return std::make_unique<DetrQueriesAdapter>(options);
}

} // namespace NeuralNetworkAdapter

MAA_VISION_NS_END
