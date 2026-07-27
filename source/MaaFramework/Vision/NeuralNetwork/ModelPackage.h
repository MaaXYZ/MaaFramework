#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include "ModelDescriptor.h"

namespace Ort
{
struct MemoryInfo;
struct Session;
}

MAA_VISION_NS_BEGIN

namespace NeuralNetwork
{

struct ModelLayerFiles
{
    std::filesystem::path onnx_path;
    bool onnx_exists = false;
    bool descriptor_exists = false;
};

class ModelPathResolver
{
public:
    static std::optional<ModelLayerFiles> select(std::span<const ModelLayerFiles> layers);
    static std::string cache_key(const std::filesystem::path& onnx_path, uint64_t backend_generation);
};

struct ModelPackage
{
    std::filesystem::path onnx_path;
    std::shared_ptr<Ort::Session> session;
    std::shared_ptr<const Ort::MemoryInfo> memory_info;
    std::shared_ptr<const IModelAdapter> adapter;
    ModelDescriptor descriptor;
    std::vector<std::string> metadata_labels;
    std::vector<std::string> input_names;
    std::vector<std::string> output_names;
};

struct ModelPackageLoadResult
{
    std::shared_ptr<const ModelPackage> package;
    std::string error;
};

} // namespace NeuralNetwork

MAA_VISION_NS_END
