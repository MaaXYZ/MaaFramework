#pragma once

#include <filesystem>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <onnxruntime/onnxruntime_cxx_api.h>

#include "Common/Conf.h"
#include "MaaUtils/NonCopyable.hpp"
#include "Vision/InferenceSession.h"

MAA_RES_NS_BEGIN

// Manages NeuralNetworkClassify / NeuralNetworkDetect sessions.
// The backend is configured by ResourceMgr when loading. The engine (onnxruntime / ncnn) is fixed once sessions are loaded,
// onnxruntime providers may still change and only affect models loaded later.
class InferenceResMgr : public NonCopyable
{
public:
    InferenceResMgr();

public:
    void use_cpu();
    void use_cuda(int device_id);
    void use_directml(int device_id);
    void use_coreml(uint32_t coreml_flag);
    bool use_webgpu(int device_id);
    // returns false if MaaFW is built without NCNN
    bool use_ncnn();

    bool lazy_load_classifier(const std::filesystem::path& path);
    bool lazy_load_detector(const std::filesystem::path& path);
    void clear();

public:
    MAA_VISION_NS::InferenceSessionPtr classifier(const std::string& name);
    MAA_VISION_NS::InferenceSessionPtr detector(const std::string& name);

private:
    MAA_VISION_NS::InferenceSessionPtr get_or_load(
        std::unordered_map<std::string, MAA_VISION_NS::InferenceSessionPtr>& cache,
        const std::string& name,
        const std::vector<std::filesystem::path>& roots);
    MAA_VISION_NS::InferenceSessionPtr load(const std::string& name, const std::vector<std::filesystem::path>& roots);
    MAA_VISION_NS::InferenceSessionPtr load_ort(const std::filesystem::path& path);
    bool sessions_loaded() const;
    bool can_switch_to(MAA_VISION_NS::InferenceEngine engine) const;

    std::vector<std::filesystem::path> classifier_roots_;
    std::vector<std::filesystem::path> detector_roots_;

    MAA_VISION_NS::InferenceEngine engine_ = MAA_VISION_NS::InferenceEngine::ONNXRuntime;
#ifdef MAA_WITH_NCNN
    int ncnn_threads_ = 0;
#endif

    Ort::Env env_ = Ort::Env(ORT_LOGGING_LEVEL_FATAL, "MaaFW");
    Ort::SessionOptions options_;
    Ort::MemoryInfo memory_info_;

    mutable std::mutex mutex_;
    std::unordered_map<std::string, MAA_VISION_NS::InferenceSessionPtr> classifiers_;
    std::unordered_map<std::string, MAA_VISION_NS::InferenceSessionPtr> detectors_;
};

MAA_RES_NS_END
