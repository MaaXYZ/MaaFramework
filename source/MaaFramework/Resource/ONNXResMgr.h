#pragma once

#include <filesystem>
#include <memory>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <vector>

#include <onnxruntime/onnxruntime_cxx_api.h>

#include "Common/Conf.h"
#include "MaaUtils/NonCopyable.hpp"
#include "Vision/NeuralNetwork/ModelPackage.h"

MAA_RES_NS_BEGIN

class ONNXResMgr : public NonCopyable
{
public:
    ONNXResMgr();

public:
    void use_cpu();
    void use_cuda(int device_id);
    void use_directml(int device_id);
    void use_coreml(uint32_t coreml_flag);

    bool lazy_load_classifier(const std::filesystem::path& path);
    bool lazy_load_detector(const std::filesystem::path& path);
    void clear();

public:
    std::shared_ptr<Ort::Session> classifier(const std::string& name);
    std::shared_ptr<Ort::Session> detector(const std::string& name);
    MAA_VISION_NS::NeuralNetwork::ModelPackageLoadResult detector_model(const std::string& name);
    const Ort::MemoryInfo& memory_info() const;

private:
    std::shared_ptr<Ort::Session> load(const std::string& name, const std::vector<std::filesystem::path>& roots);
    MAA_VISION_NS::NeuralNetwork::ModelPackageLoadResult build_detector_model(const MAA_VISION_NS::NeuralNetwork::ModelLayerFiles& layer);
    std::vector<std::string> read_metadata_labels(const Ort::Session& session) const;
    void invalidate_detector_models();

    std::vector<std::filesystem::path> classifier_roots_;
    std::vector<std::filesystem::path> detector_roots_;

    Ort::Env env_ = Ort::Env(ORT_LOGGING_LEVEL_FATAL, "MaaFW");
    Ort::SessionOptions options_;
    Ort::MemoryInfo memory_info_;

    std::unordered_map<std::string, std::shared_ptr<Ort::Session>> classifiers_;
    std::unordered_map<std::string, std::shared_ptr<const MAA_VISION_NS::NeuralNetwork::ModelPackage>> detector_models_;
    MAA_VISION_NS::NeuralNetwork::ModelAdapterRegistry adapter_registry_;
    uint64_t backend_generation_ = 0;
    std::mutex mutex_;
};

MAA_RES_NS_END
