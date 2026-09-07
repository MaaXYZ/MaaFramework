#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <vector>

#include <onnxruntime/onnxruntime_cxx_api.h>

#include "Common/Conf.h"
#include "MaaUtils/NonCopyable.hpp"
#include "Vision/NeuralNetworkDetector.h"

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
    MAA_VISION_NS::NeuralNetworkDetector::ModelLoadResult detector_model(const std::string& name);
    const Ort::MemoryInfo& memory_info() const;

private:
    enum class BackendType
    {
        CPU,
        CUDA,
        DirectML,
        CoreML,
    };

    struct BackendState
    {
        BackendType type = BackendType::CPU;
        int64_t argument = 0;

        bool operator==(const BackendState&) const = default;
    };

    std::shared_ptr<Ort::Session> load(const std::string& name, const std::vector<std::filesystem::path>& roots);
    void invalidate_detector_models();

    std::vector<std::filesystem::path> classifier_roots_;
    std::vector<std::filesystem::path> detector_roots_;

    Ort::Env env_ = Ort::Env(ORT_LOGGING_LEVEL_FATAL, "MaaFW");
    Ort::SessionOptions options_;
    Ort::MemoryInfo memory_info_;

    std::unordered_map<std::string, std::shared_ptr<Ort::Session>> classifiers_;
    std::unordered_map<std::string, MAA_VISION_NS::NeuralNetworkDetector::ModelLoadResult> detector_models_;
    BackendState backend_;
    uint64_t backend_generation_ = 0;
    std::mutex mutex_;
};

MAA_RES_NS_END
