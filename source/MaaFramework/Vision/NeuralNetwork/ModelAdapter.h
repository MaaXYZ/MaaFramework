#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <meojson/json.hpp>

#include "DetectionPostProcessor.h"
#include "ImagePreprocessor.h"

MAA_VISION_NS_BEGIN

namespace NeuralNetwork
{

struct TensorView
{
    std::string_view name;
    std::span<const float> data;
    std::vector<int64_t> shape;
};

struct TensorSpec
{
    std::string_view name;
    std::vector<int64_t> shape;
};

struct DecodedDetections
{
    std::vector<DetectionCandidate> candidates;
    std::optional<size_t> class_count;
};

struct DecodeOptions
{
    float score_floor = 0.0F;
};

class IModelAdapter
{
public:
    virtual ~IModelAdapter() = default;

    virtual std::string_view id() const = 0;
    virtual PreprocessSpec default_preprocess() const = 0;
    virtual NmsPolicy default_nms() const = 0;

    virtual float default_nms_threshold() const { return 0.7F; }

    virtual std::vector<std::string> requested_output_names() const { return { }; }

    virtual bool validate_inputs(std::span<const TensorSpec> tensors, std::string& error) const;
    virtual bool configure(const json::object& config, std::string& error) = 0;
    virtual bool validate_outputs(std::span<const TensorSpec> tensors, std::string& error) const = 0;
    virtual bool decode(
        std::span<const TensorView> tensors,
        const CoordinateTransform& transform,
        const DecodeOptions& options,
        DecodedDetections& output,
        std::string& error) const = 0;

    bool decode(std::span<const TensorView> tensors, const CoordinateTransform& transform, DecodedDetections& output, std::string& error)
        const;
    bool decode(
        std::span<const TensorView> tensors,
        const CoordinateTransform& transform,
        const DecodeOptions& options,
        std::vector<DetectionCandidate>& output,
        std::string& error) const;
    bool decode(
        std::span<const TensorView> tensors,
        const CoordinateTransform& transform,
        std::vector<DetectionCandidate>& output,
        std::string& error) const;
};

class ModelAdapterRegistry
{
public:
    using Factory = std::function<std::unique_ptr<IModelAdapter>()>;

    bool register_factory(std::string id, Factory factory);
    std::unique_ptr<IModelAdapter> create(std::string_view id) const;

private:
    std::unordered_map<std::string, Factory> factories_;
};

bool register_builtin_adapters(ModelAdapterRegistry& registry);

} // namespace NeuralNetwork

MAA_VISION_NS_END
