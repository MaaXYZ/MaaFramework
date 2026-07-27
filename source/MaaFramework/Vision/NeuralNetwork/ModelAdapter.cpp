#include "ModelAdapter.h"

MAA_VISION_NS_BEGIN

namespace NeuralNetwork
{

bool IModelAdapter::validate_inputs(std::span<const TensorSpec> tensors, std::string& error) const
{
    if (tensors.size() != 1 || tensors.front().shape.size() != 4 || (tensors.front().shape[0] != 1 && tensors.front().shape[0] != -1)
        || tensors.front().shape[1] != 3) {
        error = std::string(id()) + " input must be batch-1 NCHW with three channels";
        return false;
    }

    error.clear();
    return true;
}

bool IModelAdapter::decode(
    std::span<const TensorView> tensors,
    const CoordinateTransform& transform,
    DecodedDetections& output,
    std::string& error) const
{
    return decode(tensors, transform, DecodeOptions { }, output, error);
}

bool IModelAdapter::decode(
    std::span<const TensorView> tensors,
    const CoordinateTransform& transform,
    const DecodeOptions& options,
    std::vector<DetectionCandidate>& output,
    std::string& error) const
{
    DecodedDetections decoded;
    if (!decode(tensors, transform, options, decoded, error)) {
        return false;
    }
    output = std::move(decoded.candidates);
    return true;
}

bool IModelAdapter::decode(
    std::span<const TensorView> tensors,
    const CoordinateTransform& transform,
    std::vector<DetectionCandidate>& output,
    std::string& error) const
{
    return decode(tensors, transform, DecodeOptions { }, output, error);
}

bool ModelAdapterRegistry::register_factory(std::string id, Factory factory)
{
    if (id.empty() || !factory) {
        return false;
    }

    return factories_.emplace(std::move(id), std::move(factory)).second;
}

std::unique_ptr<IModelAdapter> ModelAdapterRegistry::create(std::string_view id) const
{
    auto iter = factories_.find(std::string(id));
    return iter == factories_.end() ? nullptr : iter->second();
}

} // namespace NeuralNetwork

MAA_VISION_NS_END
