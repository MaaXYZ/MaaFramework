#include "InferenceResMgr.h"

#include <filesystem>
#include <ranges>
#include <string>
#include <unordered_map>
#include <unordered_set>

#ifdef _WIN32
#include "MaaUtils/SafeWindows.hpp"
#endif

#include "MLProvider.h"
#include "MaaUtils/Logger.h"
#include "MaaUtils/Platform.h"
#include "NcnnSession.h"
#include "OrtSession.h"

MAA_RES_NS_BEGIN

InferenceResMgr::InferenceResMgr()
    : memory_info_(Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeDefault))
{
}

// InferenceResMgr::~InferenceResMgr()
//{
//      if (gpu_device_id_) {
//          LogWarn << "GPU is enabled, leaking resources";
//
//         // FIXME: intentionally leak ort objects to avoid crash (double free?)
//         // https://github.com/microsoft/onnxruntime/issues/15174
//         for (auto& session : classifiers_ | std::views::values) {
//             auto leak_session = new Ort::Session(nullptr);
//             *leak_session = std::move(*session);
//         }
//         for (auto& session : detectors_ | std::views::values) {
//             auto leak_session = new Ort::Session(nullptr);
//             *leak_session = std::move(*session);
//         }
//
//         auto leak_options = new Ort::SessionOptions(nullptr);
//         *leak_options = std::move(options_);
//     }
// }

void InferenceResMgr::use_cpu()
{
    if (!can_switch_to(MAA_VISION_NS::InferenceEngine::ONNXRuntime)) {
        return;
    }
    engine_ = MAA_VISION_NS::InferenceEngine::ONNXRuntime;

    LogInfo;

    options_ = { };
    memory_info_ = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeDefault);
}

void InferenceResMgr::use_cuda(int device_id)
{
    if (!can_switch_to(MAA_VISION_NS::InferenceEngine::ONNXRuntime)) {
        return;
    }
    engine_ = MAA_VISION_NS::InferenceEngine::ONNXRuntime;

    LogInfo << VAR(device_id);

    options_ = { };
    OrtCUDAProviderOptions cuda_options { };
    cuda_options.device_id = device_id;
    options_.AppendExecutionProvider_CUDA(cuda_options);

    // Input tensors are created from std::vector<float> (host memory).
    // Keep CPU memory info here and let ORT move data to CUDA EP internally.
    memory_info_ = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeDefault);

    LogInfo << "Using CUDA execution provider with device_id" << device_id;
}

void InferenceResMgr::use_directml(int device_id)
{
    if (!can_switch_to(MAA_VISION_NS::InferenceEngine::ONNXRuntime)) {
        return;
    }
    engine_ = MAA_VISION_NS::InferenceEngine::ONNXRuntime;

    LogInfo << VAR(device_id);

#ifdef MAA_WITH_DML

    options_ = { };
    auto status = OrtSessionOptionsAppendExecutionProvider_DML(options_, device_id);
    if (!Ort::Status(status).IsOK()) {
        LogError << "Failed to append DML execution provider with device_id" << device_id;
        return;
    }

    memory_info_ = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeDefault);

    LogInfo << "Using DML execution provider with device_id" << device_id;

#else

    LogError << "MaaFW built without DML";

#endif
}

bool InferenceResMgr::use_webgpu(int device_id)
{
    if (!can_switch_to(MAA_VISION_NS::InferenceEngine::ONNXRuntime)) {
        return false;
    }
    engine_ = MAA_VISION_NS::InferenceEngine::ONNXRuntime;

    LogInfo << VAR(device_id);

    try {
        options_ = { };
        // OrtSessionOptionsAppendExecutionProvider prefixes each key with
        // "ep.webgpuexecutionprovider.". The EP reads "deviceId"; device 0 is the default.
        std::unordered_map<std::string, std::string> webgpu_options;
        if (device_id > 0) {
            webgpu_options.emplace("deviceId", std::to_string(device_id));
        }
        options_.AppendExecutionProvider("WebGPU", webgpu_options);
    }
    catch (const Ort::Exception& e) {
        // A CPU-only machine still reports the EP, then throws when no adapter exists.
        LogError << "Failed to append WebGPU execution provider" << e.what();
        options_ = { };
        memory_info_ = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeDefault);
        return false;
    }

    memory_info_ = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeDefault);

    LogInfo << "Using WebGPU execution provider with device_id" << device_id;
    return true;
}

void InferenceResMgr::use_coreml(uint32_t coreml_flag)
{
    if (!can_switch_to(MAA_VISION_NS::InferenceEngine::ONNXRuntime)) {
        return;
    }
    engine_ = MAA_VISION_NS::InferenceEngine::ONNXRuntime;

    LogWarn << "CoreML execution provider is deprecated and is no longer shipped. Use WebGPU instead" << VAR(coreml_flag);

#ifdef MAA_WITH_COREML

    options_ = { };
    auto status = OrtSessionOptionsAppendExecutionProvider_CoreML((OrtSessionOptions*)options_, coreml_flag);
    if (!Ort::Status(status).IsOK()) {
        LogError << "Failed to append CoreML execution provider";
    }

    memory_info_ = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeDefault);

    LogInfo << "Using CoreML execution provider";

#else

    LogError << "MaaFW built without CoreML";

#endif
}

bool InferenceResMgr::use_ncnn()
{
    if (!can_switch_to(MAA_VISION_NS::InferenceEngine::NCNN)) {
        return false;
    }

#ifdef MAA_WITH_NCNN
    engine_ = MAA_VISION_NS::InferenceEngine::NCNN;
    ncnn_threads_ = NcnnSession::default_num_threads();

    LogInfo << "Using NCNN CPU" << VAR(ncnn_threads_);
    return true;
#else
    LogError << "MaaFW built without NCNN";
    return false;
#endif
}

bool InferenceResMgr::lazy_load_classifier(const std::filesystem::path& path)
{
    LogFunc << VAR(path);

    // roots are read by get_or_load under the same mutex
    std::unique_lock lock(mutex_);
    classifier_roots_.emplace_back(path);

    return true;
}

bool InferenceResMgr::lazy_load_detector(const std::filesystem::path& path)
{
    LogFunc << VAR(path);

    std::unique_lock lock(mutex_);
    detector_roots_.emplace_back(path);

    return true;
}

void InferenceResMgr::clear()
{
    LogFunc;

    std::unique_lock lock(mutex_);

    classifier_roots_.clear();
    detector_roots_.clear();
    classifiers_.clear();
    detectors_.clear();
}

MAA_VISION_NS::InferenceSessionPtr InferenceResMgr::classifier(const std::string& name)
{
    return get_or_load(classifiers_, name, classifier_roots_);
}

MAA_VISION_NS::InferenceSessionPtr InferenceResMgr::detector(const std::string& name)
{
    return get_or_load(detectors_, name, detector_roots_);
}

MAA_VISION_NS::InferenceSessionPtr InferenceResMgr::get_or_load(
    std::unordered_map<std::string, MAA_VISION_NS::InferenceSessionPtr>& cache,
    const std::string& name,
    const std::vector<std::filesystem::path>& roots)
{
    // multiple taskers may share one resource, guard lookup + load + insert
    std::unique_lock lock(mutex_);

    if (auto iter = cache.find(name); iter != cache.end()) {
        return iter->second;
    }

    auto session = load(name, roots);
    if (session) {
        cache.emplace(name, session);
    }

    return session;
}

bool InferenceResMgr::sessions_loaded() const
{
    std::unique_lock lock(mutex_);
    return !classifiers_.empty() || !detectors_.empty();
}

bool InferenceResMgr::can_switch_to(MAA_VISION_NS::InferenceEngine engine) const
{
    // providers of the same engine may change, only models loaded later use the new one (as before).
    // sessions of different engines are never mixed, ResourceMgr refuses such a switch before clear()
    if (engine_ == engine || !sessions_loaded()) {
        return true;
    }

    LogError << "sessions of another engine already loaded, ignore" << VAR(static_cast<int>(engine_)) << VAR(static_cast<int>(engine));
    return false;
}

MAA_VISION_NS::InferenceSessionPtr InferenceResMgr::load(const std::string& name, const std::vector<std::filesystem::path>& roots)
{
    LogFunc << VAR(name) << VAR(roots);

    for (const auto& root : roots | std::views::reverse) {
        auto path = root / MAA_NS::path(name);

#ifdef MAA_WITH_NCNN
        if (engine_ == MAA_VISION_NS::InferenceEngine::NCNN) {
            // NCNN is bound to the packaged models: never load onnx with onnxruntime.
            if (NcnnSession::files_exist(path)) {
                auto session = NcnnSession::create(path, ncnn_threads_);
                if (!session) {
                    LogError << "failed to load ncnn model" << VAR(path);
                }
                return session;
            }
            if (NcnnSession::any_file_exists(path) || std::filesystem::exists(path)) {
                // this root overrides the model but does not ship a complete ncnn model
                LogError << "ncnn model is incomplete or missing, expect .ncnn.param/.ncnn.bin/.ncnn.json next to" << VAR(path);
                return nullptr;
            }
            continue;
        }
#endif

        if (!std::filesystem::exists(path)) {
            continue;
        }

        LogDebug << VAR(path);
        return load_ort(path);
    }

    LogError << "model not found" << VAR(name) << VAR(roots);
    return nullptr;
}

MAA_VISION_NS::InferenceSessionPtr InferenceResMgr::load_ort(const std::filesystem::path& path)
{
    return OrtSession::create(env_, path, options_, memory_info_);
}

MAA_RES_NS_END
