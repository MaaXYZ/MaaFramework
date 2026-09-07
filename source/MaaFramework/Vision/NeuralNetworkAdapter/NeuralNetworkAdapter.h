#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "Common/Conf.h"

MAA_VISION_NS_BEGIN

namespace NeuralNetworkAdapter
{

enum class TensorElementType
{
    Float32,
    Float16,
    Int64,
};

struct TensorSpec
{
    std::string name;
    TensorElementType element_type = TensorElementType::Float32;
    std::vector<int64_t> shape;
};

struct TensorView
{
    std::string_view name;
    TensorElementType element_type = TensorElementType::Float32;
    std::span<const float> float_data;
    std::span<const int64_t> int64_data;
    std::vector<int64_t> shape;
};

struct RawDetection
{
    std::array<float, 4> box { };
    float score = 0.0F;
    int class_id = -1;
};

struct DecodedDetections
{
    std::vector<RawDetection> candidates;
    std::optional<size_t> class_count;
};

enum class TensorLayout
{
    ChannelsFirst,
    ChannelsLast,
    Split,
};

enum class TensorField
{
    Boxes,
    ClassScores,
    Objectness,
    Score,
    ClassId,
};

enum class ScoreActivation
{
    None,
    Sigmoid,
    Softmax,
};

enum class ScoreCombination
{
    None,
    Multiply,
};

struct TensorMapSpec
{
    TensorLayout layout = TensorLayout::ChannelsLast;
    std::vector<TensorField> fields;
};

struct ScoreDecodeSpec
{
    ScoreActivation activation = ScoreActivation::None;
    ScoreActivation objectness_activation = ScoreActivation::None;
    ScoreCombination combination = ScoreCombination::None;
    std::optional<int64_t> no_object_index;
};

struct OutputPipelineSpec
{
    TensorMapSpec tensor_map;
    ScoreDecodeSpec score_decode;
    std::optional<size_t> top_k;
};

struct DecodeOptions
{
    float score_floor = 0.0F;
    bool multi_label = true;
};

class IAdapter
{
public:
    virtual ~IAdapter() = default;

    virtual bool validate(std::span<const TensorSpec> tensors, std::string& error) const = 0;
    virtual bool
        decode(std::span<const TensorView> tensors, const DecodeOptions& options, DecodedDetections& output, std::string& error) const = 0;
};

std::unique_ptr<IAdapter> create(const OutputPipelineSpec& spec);

bool has_batch_one(const std::vector<int64_t>& shape);
bool is_float_tensor(TensorElementType type);
bool check_finite(std::span<const float> values, std::string_view protocol, std::string& error);
bool decode_class_id(float value, int& class_id, std::string_view protocol, std::string& error);
bool validate_score(float value, float& score, std::string_view protocol, std::string& error);

} // namespace NeuralNetworkAdapter

MAA_VISION_NS_END
