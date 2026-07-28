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

enum class OutputProtocol
{
    YoloDense,
    YoloEndToEnd,
    DetrQueries,
    DetrPacked,
};

enum class YoloDenseLayout
{
    ChannelsFirst,
    ChannelsLast,
};

enum class DetrActivation
{
    Sigmoid,
    SoftmaxWithNoObject,
};

enum class DetrPackedLayout
{
    Interleaved,
    Split,
};

struct AdapterOptions
{
    OutputProtocol protocol = OutputProtocol::YoloDense;
    YoloDenseLayout yolo_dense_layout = YoloDenseLayout::ChannelsFirst;
    bool yolo_dense_multi_label = false;
    DetrActivation detr_activation = DetrActivation::Sigmoid;
    size_t detr_top_k = 300;
    DetrPackedLayout detr_packed_layout = DetrPackedLayout::Interleaved;
};

struct DecodeOptions
{
    float score_floor = 0.0F;
};

class IAdapter
{
public:
    virtual ~IAdapter() = default;

    virtual bool validate(std::span<const TensorSpec> tensors, std::string& error) const = 0;
    virtual bool
        decode(std::span<const TensorView> tensors, const DecodeOptions& options, DecodedDetections& output, std::string& error) const = 0;
};

std::unique_ptr<IAdapter> create(const AdapterOptions& options);

bool has_batch_one(const std::vector<int64_t>& shape);
bool check_finite(std::span<const float> values, std::string_view protocol, std::string& error);
bool decode_class_id(float value, int& class_id, std::string_view protocol, std::string& error);
bool validate_score(float value, float& score, std::string_view protocol, std::string& error);

std::unique_ptr<IAdapter> create_yolo_dense(const AdapterOptions& options);
std::unique_ptr<IAdapter> create_yolo_end_to_end();
std::unique_ptr<IAdapter> create_detr_queries(const AdapterOptions& options);
std::unique_ptr<IAdapter> create_detr_packed(const AdapterOptions& options);

} // namespace NeuralNetworkAdapter

MAA_VISION_NS_END
