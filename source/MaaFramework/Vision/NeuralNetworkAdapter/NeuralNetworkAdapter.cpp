#include "NeuralNetworkAdapter.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <ranges>
#include <unordered_set>

MAA_VISION_NS_BEGIN

namespace NeuralNetworkAdapter
{

namespace
{

bool is_scalar_field(TensorField field)
{
    return field == TensorField::Objectness || field == TensorField::Score || field == TensorField::ClassId;
}

size_t fixed_field_width(TensorField field)
{
    return field == TensorField::Boxes ? 4 : (is_scalar_field(field) ? 1 : 0);
}

bool compatible_dimension(int64_t lhs, int64_t rhs)
{
    return lhs == -1 || rhs == -1 || lhs == rhs;
}

bool valid_count_dimension(int64_t value)
{
    return value == -1 || value > 0;
}

float sigmoid(float value)
{
    return value >= 0.0F ? 1.0F / (1.0F + std::exp(-value)) : std::exp(value) / (1.0F + std::exp(value));
}

struct Mapping
{
    size_t proposal_count = 0;
    size_t class_score_count = 0;
    std::vector<size_t> offsets;
};

struct ScoredDetection
{
    RawDetection detection;
    size_t order = 0;
};

class GenericDetectionAdapter final : public IAdapter
{
public:
    explicit GenericDetectionAdapter(OutputPipelineSpec spec)
        : spec_(std::move(spec))
    {
    }

    bool validate(std::span<const TensorSpec> tensors, std::string& error) const override
    {
        if (!validate_semantics(error)) {
            return false;
        }
        if (spec_.tensor_map.layout == TensorLayout::Split) {
            return validate_split(tensors, error);
        }
        return validate_interleaved(tensors, error);
    }

    bool decode(std::span<const TensorView> tensors, const DecodeOptions& options, DecodedDetections& output, std::string& error)
        const override
    {
        std::vector<TensorSpec> runtime_specs;
        runtime_specs.reserve(tensors.size());
        for (const auto& tensor : tensors) {
            runtime_specs.emplace_back(
                TensorSpec {
                    .name = std::string(tensor.name),
                    .element_type = tensor.element_type,
                    .shape = tensor.shape,
                });
        }
        if (!validate(runtime_specs, error)) {
            error = "detector outputs changed at runtime: " + error;
            return false;
        }

        for (const auto& tensor : tensors) {
            if (is_float_tensor(tensor.element_type) && !check_finite(tensor.float_data, "detector", error)) {
                return false;
            }
        }

        Mapping mapping;
        if (!make_mapping(tensors, mapping, error)) {
            return false;
        }

        output = { };
        std::vector<ScoredDetection> scored;
        if (has_field(TensorField::ClassScores)) {
            if (!decode_class_scores(tensors, mapping, options, scored, output.class_count, error)) {
                return false;
            }
        }
        else if (!decode_packed(tensors, mapping, options, scored, error)) {
            return false;
        }

        const size_t selected_count = std::min(spec_.top_k.value_or(scored.size()), scored.size());
        if (selected_count < scored.size()) {
            std::ranges::partial_sort(
                scored,
                scored.begin() + static_cast<std::ptrdiff_t>(selected_count),
                [](const ScoredDetection& lhs, const ScoredDetection& rhs) {
                    return lhs.detection.score != rhs.detection.score ? lhs.detection.score > rhs.detection.score : lhs.order < rhs.order;
                });
            scored.resize(selected_count);
        }
        output.candidates.reserve(scored.size());
        std::ranges::transform(scored, std::back_inserter(output.candidates), &ScoredDetection::detection);
        error.clear();
        return true;
    }

private:
    bool has_field(TensorField field) const { return std::ranges::find(spec_.tensor_map.fields, field) != spec_.tensor_map.fields.end(); }

    size_t field_index(TensorField field) const
    {
        return static_cast<size_t>(std::ranges::find(spec_.tensor_map.fields, field) - spec_.tensor_map.fields.begin());
    }

    bool validate_semantics(std::string& error) const
    {
        if (spec_.tensor_map.fields.empty()) {
            error = "TensorMap fields must not be empty";
            return false;
        }
        std::unordered_set<TensorField> unique;
        for (const auto field : spec_.tensor_map.fields) {
            if (!unique.emplace(field).second) {
                error = "TensorMap fields must not contain duplicate roles";
                return false;
            }
        }
        if (!has_field(TensorField::Boxes)) {
            error = "TensorMap must contain Boxes";
            return false;
        }

        const bool class_scores = has_field(TensorField::ClassScores);
        const bool score = has_field(TensorField::Score);
        const bool class_id = has_field(TensorField::ClassId);
        if (class_scores == (score && class_id)) {
            error = "TensorMap must contain either ClassScores or both Score and ClassId";
            return false;
        }
        if (class_scores && (score || class_id)) {
            error = "ClassScores cannot be combined with Score or ClassId";
            return false;
        }
        if (!class_scores && (spec_.score_decode.no_object_index || has_field(TensorField::Objectness))) {
            error = "no-object and objectness settings require ClassScores";
            return false;
        }
        const bool objectness = has_field(TensorField::Objectness);
        if (objectness != (spec_.score_decode.combination == ScoreCombination::Multiply)) {
            error = "Objectness requires ScoreDecode combine Multiply";
            return false;
        }
        if (!objectness && spec_.score_decode.objectness_activation != ScoreActivation::None) {
            error = "objectness activation requires an Objectness field";
            return false;
        }
        if (spec_.score_decode.objectness_activation == ScoreActivation::Softmax) {
            error = "objectness activation does not support Softmax";
            return false;
        }
        if (spec_.top_k && *spec_.top_k == 0) {
            error = "CandidateSelect top_k must be positive";
            return false;
        }
        error.clear();
        return true;
    }

    bool validate_interleaved(std::span<const TensorSpec> tensors, std::string& error) const
    {
        if (tensors.size() != 1 || !is_float_tensor(tensors.front().element_type)) {
            error = "interleaved TensorMap requires exactly one floating-point output";
            return false;
        }
        const auto& shape = tensors.front().shape;
        if (shape.size() != 3 || !has_batch_one(shape)) {
            error = "interleaved TensorMap output must have rank three and batch one";
            return false;
        }
        const int64_t features = spec_.tensor_map.layout == TensorLayout::ChannelsFirst ? shape[1] : shape[2];
        const int64_t proposals = spec_.tensor_map.layout == TensorLayout::ChannelsFirst ? shape[2] : shape[1];
        if (!valid_count_dimension(features) || !valid_count_dimension(proposals)) {
            error = "interleaved TensorMap output has an invalid shape";
            return false;
        }
        size_t fixed_width = 0;
        for (const auto field : spec_.tensor_map.fields) {
            fixed_width += fixed_field_width(field);
        }
        const bool variable_width = has_field(TensorField::ClassScores);
        if (features != -1
            && (features < static_cast<int64_t>(fixed_width) || (variable_width && features == static_cast<int64_t>(fixed_width))
                || (!variable_width && features != static_cast<int64_t>(fixed_width)))) {
            error = "interleaved TensorMap feature count does not match its fields";
            return false;
        }
        error.clear();
        return true;
    }

    bool validate_split(std::span<const TensorSpec> tensors, std::string& error) const
    {
        if (tensors.size() != spec_.tensor_map.fields.size()) {
            error = "Split TensorMap output count does not match its fields";
            return false;
        }
        int64_t proposal_count = -1;
        for (size_t index = 0; index < tensors.size(); ++index) {
            const auto field = spec_.tensor_map.fields[index];
            const auto& tensor = tensors[index];
            const auto& shape = tensor.shape;
            if (field == TensorField::ClassId) {
                if (tensor.element_type != TensorElementType::Int64 && !is_float_tensor(tensor.element_type)) {
                    error = "ClassId output must be INT64, FP32, or FP16";
                    return false;
                }
            }
            else if (!is_float_tensor(tensor.element_type)) {
                error = "non-ClassId outputs must be FP32 or FP16";
                return false;
            }

            int64_t current_count = -1;
            if (field == TensorField::Boxes || field == TensorField::ClassScores) {
                const int64_t minimum_width = field == TensorField::Boxes ? 4 : 1;
                if (shape.size() != 3 || !has_batch_one(shape) || !valid_count_dimension(shape[1])
                    || (shape[2] != -1 && shape[2] < minimum_width) || (field == TensorField::Boxes && shape[2] != 4)) {
                    error = field == TensorField::Boxes ? "Boxes output must have shape [1, N, 4]"
                                                        : "ClassScores output must have shape [1, N, C]";
                    return false;
                }
                current_count = shape[1];
            }
            else {
                const bool rank_two = shape.size() == 2 && has_batch_one(shape) && valid_count_dimension(shape[1]);
                const bool rank_three = shape.size() == 3 && has_batch_one(shape) && valid_count_dimension(shape[1]) && shape[2] == 1;
                if (!rank_two && !rank_three) {
                    error = "scalar field output must have shape [1, N] or [1, N, 1]";
                    return false;
                }
                current_count = shape[1];
            }
            if (proposal_count != -1 && !compatible_dimension(proposal_count, current_count)) {
                error = "Split TensorMap proposal counts differ";
                return false;
            }
            if (proposal_count == -1) {
                proposal_count = current_count;
            }
        }
        error.clear();
        return true;
    }

    template <typename Tensor>
    bool make_mapping(std::span<const Tensor> tensors, Mapping& mapping, std::string& error) const
    {
        mapping = { };
        mapping.offsets.resize(spec_.tensor_map.fields.size());
        if (spec_.tensor_map.layout == TensorLayout::Split) {
            mapping.proposal_count = static_cast<size_t>(tensors.front().shape[1]);
            if (has_field(TensorField::ClassScores)) {
                mapping.class_score_count = static_cast<size_t>(tensors[field_index(TensorField::ClassScores)].shape[2]);
            }
            error.clear();
            return true;
        }

        const auto& shape = tensors.front().shape;
        const size_t feature_count = static_cast<size_t>(spec_.tensor_map.layout == TensorLayout::ChannelsFirst ? shape[1] : shape[2]);
        mapping.proposal_count = static_cast<size_t>(spec_.tensor_map.layout == TensorLayout::ChannelsFirst ? shape[2] : shape[1]);
        size_t fixed_width = 0;
        for (const auto field : spec_.tensor_map.fields) {
            fixed_width += fixed_field_width(field);
        }
        mapping.class_score_count = has_field(TensorField::ClassScores) ? feature_count - fixed_width : 0;
        size_t offset = 0;
        for (size_t index = 0; index < spec_.tensor_map.fields.size(); ++index) {
            mapping.offsets[index] = offset;
            offset += spec_.tensor_map.fields[index] == TensorField::ClassScores ? mapping.class_score_count
                                                                                 : fixed_field_width(spec_.tensor_map.fields[index]);
        }
        const size_t expected_elements = feature_count * mapping.proposal_count;
        if (tensors.front().float_data.size() != expected_elements) {
            error = "interleaved TensorMap output has an invalid element count";
            return false;
        }
        error.clear();
        return true;
    }

    float
        float_value(std::span<const TensorView> tensors, const Mapping& mapping, TensorField field, size_t proposal, size_t component) const
    {
        const size_t index = field_index(field);
        if (spec_.tensor_map.layout == TensorLayout::Split) {
            const auto& tensor = tensors[index];
            const size_t width = field == TensorField::Boxes ? 4 : (field == TensorField::ClassScores ? mapping.class_score_count : 1);
            return tensor.float_data[proposal * width + component];
        }
        const auto& tensor = tensors.front();
        const size_t feature_count = spec_.tensor_map.layout == TensorLayout::ChannelsFirst ? static_cast<size_t>(tensor.shape[1])
                                                                                            : static_cast<size_t>(tensor.shape[2]);
        const size_t feature = mapping.offsets[index] + component;
        return spec_.tensor_map.layout == TensorLayout::ChannelsFirst ? tensor.float_data[feature * mapping.proposal_count + proposal]
                                                                      : tensor.float_data[proposal * feature_count + feature];
    }

    bool class_id_value(std::span<const TensorView> tensors, const Mapping& mapping, size_t proposal, int& class_id, std::string& error)
        const
    {
        const size_t index = field_index(TensorField::ClassId);
        if (spec_.tensor_map.layout == TensorLayout::Split && tensors[index].element_type == TensorElementType::Int64) {
            const int64_t value = tensors[index].int64_data[proposal];
            if (value < 0 || value > std::numeric_limits<int>::max()) {
                error = "detector ClassId is outside the supported range";
                return false;
            }
            class_id = static_cast<int>(value);
            return true;
        }
        return decode_class_id(float_value(tensors, mapping, TensorField::ClassId, proposal, 0), class_id, "detector", error);
    }

    std::array<float, 4> box_value(std::span<const TensorView> tensors, const Mapping& mapping, size_t proposal) const
    {
        return {
            float_value(tensors, mapping, TensorField::Boxes, proposal, 0),
            float_value(tensors, mapping, TensorField::Boxes, proposal, 1),
            float_value(tensors, mapping, TensorField::Boxes, proposal, 2),
            float_value(tensors, mapping, TensorField::Boxes, proposal, 3),
        };
    }

    bool activate_scalar(float raw, ScoreActivation activation, float& output, std::string& error) const
    {
        if (activation == ScoreActivation::Sigmoid) {
            output = sigmoid(raw);
            return true;
        }
        if (activation == ScoreActivation::Softmax) {
            error = "Softmax requires a ClassScores vector";
            return false;
        }
        return validate_score(raw, output, "detector", error);
    }

    bool decode_class_scores(
        std::span<const TensorView> tensors,
        const Mapping& mapping,
        const DecodeOptions& options,
        std::vector<ScoredDetection>& output,
        std::optional<size_t>& class_count_output,
        std::string& error) const
    {
        if (mapping.class_score_count == 0 || mapping.class_score_count > static_cast<size_t>(std::numeric_limits<int>::max())) {
            error = "detector ClassScores count is outside the supported range";
            return false;
        }
        std::optional<size_t> no_object;
        if (spec_.score_decode.no_object_index) {
            const int64_t configured = *spec_.score_decode.no_object_index;
            const int64_t resolved = configured == -1 ? static_cast<int64_t>(mapping.class_score_count) - 1 : configured;
            if (resolved < 0 || resolved >= static_cast<int64_t>(mapping.class_score_count)) {
                error = "ScoreDecode no_object_index is outside ClassScores";
                return false;
            }
            no_object = static_cast<size_t>(resolved);
        }
        const size_t class_count = mapping.class_score_count - (no_object ? 1 : 0);
        if (class_count == 0) {
            error = "ScoreDecode removes the only class score";
            return false;
        }
        class_count_output = class_count;

        output.reserve(options.multi_label ? mapping.proposal_count * class_count : mapping.proposal_count);
        std::vector<float> scores(mapping.class_score_count);
        for (size_t proposal = 0; proposal < mapping.proposal_count; ++proposal) {
            if (spec_.score_decode.activation == ScoreActivation::Softmax) {
                float maximum = -std::numeric_limits<float>::infinity();
                for (size_t index = 0; index < mapping.class_score_count; ++index) {
                    scores[index] = float_value(tensors, mapping, TensorField::ClassScores, proposal, index);
                    maximum = std::max(maximum, scores[index]);
                }
                float denominator = 0.0F;
                for (auto& score : scores) {
                    score = std::exp(score - maximum);
                    denominator += score;
                }
                if (!std::isfinite(denominator) || denominator <= 0.0F) {
                    error = "ScoreDecode softmax denominator is invalid";
                    return false;
                }
                for (auto& score : scores) {
                    score /= denominator;
                }
            }
            else {
                for (size_t index = 0; index < mapping.class_score_count; ++index) {
                    if (!activate_scalar(
                            float_value(tensors, mapping, TensorField::ClassScores, proposal, index),
                            spec_.score_decode.activation,
                            scores[index],
                            error)) {
                        return false;
                    }
                }
            }

            float objectness = 1.0F;
            if (has_field(TensorField::Objectness)
                && !activate_scalar(
                    float_value(tensors, mapping, TensorField::Objectness, proposal, 0),
                    spec_.score_decode.objectness_activation,
                    objectness,
                    error)) {
                return false;
            }

            float best_score = -std::numeric_limits<float>::infinity();
            int best_class = -1;
            size_t logical_class = 0;
            for (size_t raw_class = 0; raw_class < mapping.class_score_count; ++raw_class) {
                if (no_object == raw_class) {
                    continue;
                }
                const float score = scores[raw_class] * objectness;
                if (options.multi_label) {
                    if (score >= options.score_floor) {
                        output.emplace_back(
                            ScoredDetection {
                                .detection =
                                    RawDetection {
                                        .box = box_value(tensors, mapping, proposal),
                                        .score = score,
                                        .class_id = static_cast<int>(logical_class),
                                    },
                                .order = proposal * class_count + logical_class,
                            });
                    }
                }
                else if (score > best_score) {
                    best_score = score;
                    best_class = static_cast<int>(logical_class);
                }
                ++logical_class;
            }
            if (!options.multi_label && best_score >= options.score_floor) {
                output.emplace_back(
                    ScoredDetection {
                        .detection =
                            RawDetection {
                                .box = box_value(tensors, mapping, proposal),
                                .score = best_score,
                                .class_id = best_class,
                            },
                        .order = proposal * class_count + static_cast<size_t>(best_class),
                    });
            }
        }
        error.clear();
        return true;
    }

    bool decode_packed(
        std::span<const TensorView> tensors,
        const Mapping& mapping,
        const DecodeOptions& options,
        std::vector<ScoredDetection>& output,
        std::string& error) const
    {
        output.reserve(mapping.proposal_count);
        for (size_t proposal = 0; proposal < mapping.proposal_count; ++proposal) {
            float score = 0.0F;
            int class_id = -1;
            if (!activate_scalar(
                    float_value(tensors, mapping, TensorField::Score, proposal, 0),
                    spec_.score_decode.activation,
                    score,
                    error)
                || !class_id_value(tensors, mapping, proposal, class_id, error)) {
                return false;
            }
            if (score >= options.score_floor) {
                output.emplace_back(
                    ScoredDetection {
                        .detection =
                            RawDetection {
                                .box = box_value(tensors, mapping, proposal),
                                .score = score,
                                .class_id = class_id,
                            },
                        .order = proposal,
                    });
            }
        }
        error.clear();
        return true;
    }

    OutputPipelineSpec spec_;
};

} // namespace

std::unique_ptr<IAdapter> create(const OutputPipelineSpec& spec)
{
    return std::make_unique<GenericDetectionAdapter>(spec);
}

bool has_batch_one(const std::vector<int64_t>& shape)
{
    return !shape.empty() && (shape.front() == 1 || shape.front() == -1);
}

bool is_float_tensor(TensorElementType type)
{
    return type == TensorElementType::Float32 || type == TensorElementType::Float16;
}

bool check_finite(std::span<const float> values, std::string_view protocol, std::string& error)
{
    if (std::ranges::all_of(values, [](float value) { return std::isfinite(value); })) {
        return true;
    }
    error = std::string(protocol) + " output contains non-finite values";
    return false;
}

bool decode_class_id(float value, int& class_id, std::string_view protocol, std::string& error)
{
    const double precise_value = value;
    const double rounded = std::round(precise_value);
    if (!std::isfinite(precise_value) || precise_value < 0.0 || std::abs(precise_value - rounded) > 1.0e-5
        || rounded > static_cast<double>(std::numeric_limits<int>::max())) {
        error = std::string(protocol) + " class id must be a non-negative integer";
        return false;
    }
    class_id = static_cast<int>(rounded);
    return true;
}

bool validate_score(float value, float& score, std::string_view protocol, std::string& error)
{
    constexpr float kTolerance = 1.0e-5F;
    if (!std::isfinite(value) || value < -kTolerance || value > 1.0F + kTolerance) {
        error = std::string(protocol) + " score is outside [0, 1]";
        return false;
    }
    score = std::clamp(value, 0.0F, 1.0F);
    return true;
}

} // namespace NeuralNetworkAdapter

MAA_VISION_NS_END
