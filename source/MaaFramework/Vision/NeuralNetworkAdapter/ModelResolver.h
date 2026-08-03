#pragma once

#include <array>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <meojson/json.hpp>

#include "../VisionTypes.h"
#include "MaaUtils/NoWarningCVMat.hpp"
#include "NeuralNetworkAdapter.h"

MAA_VISION_NS_BEGIN

namespace NeuralNetworkAdapter
{

enum class ResizeMode
{
    Stretch,
    Letterbox,
};

enum class ResizeInterpolation
{
    Nearest,
    Linear,
    Cubic,
    Area,
};

enum class PaddingPosition
{
    Center,
    TopLeft,
};

enum class ColorOrder
{
    RGB,
    BGR,
};

enum class ImageLayout
{
    NCHW,
    NHWC,
};

enum class OriginalSizeOrder
{
    HW,
    WH,
};

enum class BoxFormat
{
    CxCyWh,
    XyXy,
};

enum class CoordinateSpace
{
    Input,
    Normalized,
    Source,
};

struct OriginalSizeInput
{
    std::string name;
    OriginalSizeOrder order = OriginalSizeOrder::HW;
};

struct InputPipelineSpec
{
    std::string name;
    TensorElementType element_type = TensorElementType::Float32;
    std::vector<int64_t> graph_shape;
    ResizeMode resize_mode = ResizeMode::Stretch;
    std::optional<cv::Size> resize_size;
    ResizeInterpolation interpolation = ResizeInterpolation::Linear;
    std::array<float, 3> fill = { 114.0F, 114.0F, 114.0F };
    PaddingPosition padding_position = PaddingPosition::Center;
    ColorOrder color = ColorOrder::RGB;
    float scale = 1.0F / 255.0F;
    std::array<float, 3> mean = { 0.0F, 0.0F, 0.0F };
    std::array<float, 3> std = { 1.0F, 1.0F, 1.0F };
    ImageLayout layout = ImageLayout::NCHW;
    std::optional<OriginalSizeInput> original_size;
};

struct BoxDecodeSpec
{
    BoxFormat format = BoxFormat::CxCyWh;
    CoordinateSpace coordinates = CoordinateSpace::Input;
};

struct OutputPipeline
{
    OutputPipelineSpec adapter;
    BoxDecodeSpec box_decode;
};

struct ModelDescriptor
{
    InputPipelineSpec input;
    OutputPipeline output;
    std::vector<std::string> labels;
    NeuralNetwork::NmsPolicy nms = NeuralNetwork::NmsPolicy::ClassAwareIoU;
    float nms_threshold = 0.7F;
};

struct ModelFacts
{
    std::vector<TensorSpec> inputs;
    std::vector<TensorSpec> outputs;
    std::unordered_map<std::string, std::string> metadata;
};

struct ResolvedModel
{
    ModelDescriptor descriptor;
    std::vector<std::string> input_names;
    std::vector<TensorSpec> outputs;
    std::vector<std::string> metadata_labels;
};

bool resolve_model(const ModelFacts& facts, const json::value* descriptor_json, ResolvedModel& output, std::string& error);
bool validate_labels(const std::vector<std::string>& labels, std::string& error);

} // namespace NeuralNetworkAdapter

MAA_VISION_NS_END
