#include "ONNXResMgr.h"

#include <filesystem>
#include <ranges>
#include <unordered_set>

#ifdef _WIN32
#include "Utils/SafeWindows.hpp"
#endif

#if __has_include(<onnxruntime/dml_provider_factory.h>)
#define MAA_WITH_DML
#include <onnxruntime/dml_provider_factory.h>
#endif

#if __has_include(<onnxruntime/coreml_provider_factory.h>)
#define MAA_WITH_COREML
#include <onnxruntime/coreml_provider_factory.h>
#endif

#include "Utils/Logger.h"
#include "Utils/Platform.h"

MAA_RES_NS_BEGIN

// ONNXResMgr::~ONNXResMgr()
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

bool ONNXResMgr::use_cpu()
{
    LogInfo;

    options_ = {};
    gpu_device_id_ = std::nullopt;
    return true;
}

bool ONNXResMgr::use_gpu(int device_id)
{
    LogInfo << VAR(device_id);

    if (gpu_device_id_ && *gpu_device_id_ == device_id) {
        LogWarn << "GPU is already enabled";
        return true;
    }
    options_ = {};
    gpu_device_id_ = std::nullopt;

    auto all_providers_vec = Ort::GetAvailableProviders();
    std::unordered_set<std::string> all_providers(
        std::make_move_iterator(all_providers_vec.begin()),
        std::make_move_iterator(all_providers_vec.end()));
    LogInfo << VAR(all_providers);

    if (all_providers.contains("CUDAExecutionProvider")) {
        OrtCUDAProviderOptions cuda_options {};
        cuda_options.device_id = device_id;
        options_.AppendExecutionProvider_CUDA(cuda_options);

        LogInfo << "Using CUDA execution provider with device_id " << device_id;
    }
#ifdef MAA_WITH_DML
    else if (all_providers.contains("DmlExecutionProvider")) {
        auto status = OrtSessionOptionsAppendExecutionProvider_DML(options_, device_id);
        if (!Ort::Status(status).IsOK()) {
            LogError << "Failed to append DML execution provider with device_id " << device_id;
            return false;
        }
        LogInfo << "Using DML execution provider with device_id " << device_id;
    }
#endif
#ifdef MAA_WITH_COREML
    else if (all_providers.contains("CoreMLExecutionProvider")) {
        auto status = OrtSessionOptionsAppendExecutionProvider_CoreML((OrtSessionOptions*)options_, 0);
        if (!Ort::Status(status).IsOK()) {
            LogError << "Failed to append CoreML execution provider";
            return false;
        }
        LogInfo << "Using CoreML execution provider";
    }
#endif
    else {
        LogError << "No supported execution provider found";
        return false;
    }

    gpu_device_id_ = device_id;
    return true;
}

bool ONNXResMgr::lazy_load(const std::filesystem::path& path, bool is_base)
{
    LogFunc << VAR(path) << VAR(is_base);

    if (is_base) {
        clear();
    }

    classifier_roots_.emplace_back(path / kClassifierDir);
    detector_roots_.emplace_back(path / kDetectorDir);

    return true;
}

void ONNXResMgr::clear()
{
    LogFunc;

    classifier_roots_.clear();
    detector_roots_.clear();
    classifiers_.clear();
    detectors_.clear();
}

std::shared_ptr<Ort::Session> ONNXResMgr::classifier(const std::string& name) const
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

std::shared_ptr<Ort::Session> ONNXResMgr::detector(const std::string& name) const
{
    if (auto iter = detectors_.find(name); iter != detectors_.end()) {
        return iter->second;
    }

    auto session = load(name, detector_roots_);
    if (session) {
        detectors_.emplace(name, session);
    }

    return session;
}

std::shared_ptr<Ort::Session> ONNXResMgr::load(const std::string& name, const std::vector<std::filesystem::path>& roots) const
{
    LogFunc << VAR(name) << VAR(roots);

    for (const auto& root : roots | std::views::reverse) {
        auto path = root / MAA_NS::path(name);
        if (!std::filesystem::exists(path)) {
            continue;
        }

        LogTrace << VAR(path);
        Ort::Session session(env_, path.c_str(), options_);
        return std::make_shared<Ort::Session>(std::move(session));
    }

    return nullptr;
}

MAA_RES_NS_END
