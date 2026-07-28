#include "ONNXResMgr.h"

#include <filesystem>
#include <ranges>

#ifdef _WIN32
#include "MaaUtils/SafeWindows.hpp"
#endif

#include "MLProvider.h"
#include "MaaUtils/Logger.h"
#include "MaaUtils/Platform.h"

MAA_RES_NS_BEGIN

ONNXResMgr::ONNXResMgr()
    : memory_info_(Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeDefault))
{
}

void ONNXResMgr::use_cpu()
{
    LogInfo;
    std::scoped_lock lock(mutex_);

    options_ = { };
    memory_info_ = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeDefault);
    invalidate_detector_models();
}

void ONNXResMgr::use_cuda(int device_id)
{
    LogInfo << VAR(device_id);
    std::scoped_lock lock(mutex_);

    Ort::SessionOptions options;
    OrtCUDAProviderOptions cuda_options { };
    cuda_options.device_id = device_id;
    options.AppendExecutionProvider_CUDA(cuda_options);

    // Input tensors are created from std::vector<float> (host memory).
    // Keep CPU memory info here and let ORT move data to CUDA EP internally.
    options_ = std::move(options);
    memory_info_ = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeDefault);
    invalidate_detector_models();

    LogInfo << "Using CUDA execution provider with device_id" << device_id;
}

void ONNXResMgr::use_directml(int device_id)
{
    LogInfo << VAR(device_id);
    std::scoped_lock lock(mutex_);

#ifdef MAA_WITH_DML

    Ort::SessionOptions options;
    auto status = OrtSessionOptionsAppendExecutionProvider_DML(options, device_id);
    if (!Ort::Status(status).IsOK()) {
        LogError << "Failed to append DML execution provider with device_id" << device_id;
        return;
    }

    options_ = std::move(options);
    memory_info_ = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeDefault);
    invalidate_detector_models();

    LogInfo << "Using DML execution provider with device_id" << device_id;

#else

    LogError << "MaaFW built without DML";

#endif
}

void ONNXResMgr::use_coreml(uint32_t coreml_flag)
{
    LogInfo << VAR(coreml_flag);
    std::scoped_lock lock(mutex_);

#ifdef MAA_WITH_COREML

    Ort::SessionOptions options;
    auto status = OrtSessionOptionsAppendExecutionProvider_CoreML((OrtSessionOptions*)options, coreml_flag);
    if (!Ort::Status(status).IsOK()) {
        LogError << "Failed to append CoreML execution provider";
        return;
    }

    options_ = std::move(options);
    memory_info_ = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeDefault);
    invalidate_detector_models();

    LogInfo << "Using CoreML execution provider";

#else

    LogError << "MaaFW built without CoreML";

#endif
}

bool ONNXResMgr::lazy_load_classifier(const std::filesystem::path& path)
{
    LogFunc << VAR(path);

    classifier_roots_.emplace_back(path);

    return true;
}

bool ONNXResMgr::lazy_load_detector(const std::filesystem::path& path)
{
    LogFunc << VAR(path);
    std::scoped_lock lock(mutex_);

    detector_roots_.emplace_back(path);
    detector_models_.clear();

    return true;
}

void ONNXResMgr::clear()
{
    LogFunc;
    std::scoped_lock lock(mutex_);

    classifier_roots_.clear();
    detector_roots_.clear();
    classifiers_.clear();
    detector_models_.clear();
}

std::shared_ptr<Ort::Session> ONNXResMgr::classifier(const std::string& name)
{
    if (auto iter = classifiers_.find(name); iter != classifiers_.end()) {
        return iter->second;
    }

    auto session = load(name, classifier_roots_);
    if (session) {
        classifiers_.emplace(name, session);
    }

    return session;
}

MAA_VISION_NS::NeuralNetworkDetector::ModelLoadResult ONNXResMgr::detector_model(const std::string& name)
{
    std::scoped_lock lock(mutex_);

    struct Layer
    {
        std::filesystem::path onnx_path;
        bool onnx_exists = false;
        bool descriptor_exists = false;
    };

    std::vector<Layer> layers;
    layers.reserve(detector_roots_.size());
    for (const auto& root : detector_roots_) {
        const std::filesystem::path onnx_path = root / MAA_NS::path(name);
        auto descriptor_path = onnx_path;
        descriptor_path.replace_extension(".json");
        std::error_code error;
        auto absolute_path = std::filesystem::absolute(onnx_path, error);
        if (error) {
            return { .error = "failed to resolve detector model path: " + error.message() };
        }
        const bool onnx_exists = std::filesystem::exists(onnx_path, error);
        if (error) {
            return { .error = "failed to inspect detector model path: " + error.message() };
        }
        const bool descriptor_exists = std::filesystem::exists(descriptor_path, error);
        if (error) {
            return { .error = "failed to inspect detector descriptor path: " + error.message() };
        }
        layers.emplace_back(
            Layer {
                .onnx_path = absolute_path.lexically_normal(),
                .onnx_exists = onnx_exists,
                .descriptor_exists = descriptor_exists,
            });
    }

    std::optional<size_t> selected_index;
    for (size_t index = layers.size(); index > 0; --index) {
        if (layers[index - 1].onnx_exists) {
            selected_index = index - 1;
            break;
        }
    }

    if (!selected_index) {
        const std::string key = "missing:" + name + '#' + std::to_string(backend_generation_);
        if (auto iterator = detector_models_.find(key); iterator != detector_models_.end()) {
            return iterator->second;
        }
        auto result = MAA_VISION_NS::NeuralNetworkDetector::ModelLoadResult { .error = "detector model not found: " + name };
        detector_models_.emplace(key, result);
        return result;
    }

    for (size_t index = *selected_index + 1; index < layers.size(); ++index) {
        if (!layers[index].onnx_exists && layers[index].descriptor_exists) {
            auto descriptor_path = layers[index].onnx_path;
            descriptor_path.replace_extension(".json");
            LogWarn << "Ignoring detector sidecar without a model in the same Bundle layer" << VAR(descriptor_path);
        }
    }

    const auto& selected = layers[*selected_index];
    const std::string key = selected.onnx_path.generic_string() + '#' + std::to_string(backend_generation_);
    if (auto iter = detector_models_.find(key); iter != detector_models_.end()) {
        return iter->second;
    }

    MAA_VISION_NS::NeuralNetworkDetector::ModelLoadResult result;
    try {
        auto session = std::make_shared<Ort::Session>(env_, selected.onnx_path.c_str(), options_);
        result = MAA_VISION_NS::NeuralNetworkDetector::load_model(selected.onnx_path, std::move(session));
    }
    catch (const Ort::Exception& exception) {
        result.error = std::string("ONNX Runtime error: ") + exception.what();
    }
    catch (const std::exception& exception) {
        result.error = std::string("failed to create detector Session: ") + exception.what();
    }
    detector_models_.emplace(key, result);
    return result;
}

const Ort::MemoryInfo& ONNXResMgr::memory_info() const
{
    return memory_info_;
}

std::shared_ptr<Ort::Session> ONNXResMgr::load(const std::string& name, const std::vector<std::filesystem::path>& roots)
{
    LogFunc << VAR(name) << VAR(roots);

    for (const auto& root : roots | std::views::reverse) {
        auto path = root / MAA_NS::path(name);
        if (!std::filesystem::exists(path)) {
            continue;
        }

        LogDebug << VAR(path);
        Ort::Session session(env_, path.c_str(), options_);
        return std::make_shared<Ort::Session>(std::move(session));
    }

    return nullptr;
}

void ONNXResMgr::invalidate_detector_models()
{
    ++backend_generation_;
    detector_models_.clear();
}

MAA_RES_NS_END
