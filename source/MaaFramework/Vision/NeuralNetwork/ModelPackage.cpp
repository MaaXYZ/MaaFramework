#include "ModelPackage.h"

#include <ranges>

MAA_VISION_NS_BEGIN

namespace NeuralNetwork
{

std::optional<ModelLayerFiles> ModelPathResolver::select(std::span<const ModelLayerFiles> layers)
{
    for (const auto& layer : layers | std::views::reverse) {
        if (layer.onnx_exists) {
            return layer;
        }
    }
    return std::nullopt;
}

std::string ModelPathResolver::cache_key(const std::filesystem::path& onnx_path, uint64_t backend_generation)
{
    return onnx_path.lexically_normal().generic_string() + '#' + std::to_string(backend_generation);
}

} // namespace NeuralNetwork

MAA_VISION_NS_END
