#include "ONNXResMgr.h"

#include <filesystem>
#include <ranges>
#include <unordered_set>

#ifdef _WIN32
#include "Utils/SafeWindows.hpp"
#endif

#include "MLProvider.h"
#include "Utils/Logger.h"
#include "Utils/Platform.h"

MAA_RES_NS_BEGIN

ONNXResMgr::ONNXResMgr()
    : memory_info_(Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeDefault))
{
}

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

void ONNXResMgr::use_cpu()
{
    LogInfo;

    options_ = {};
    memory_info_ = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeDefault);
}

void ONNXResMgr::use_cuda(int device_id)
{
    LogInfo << VAR(device_id);

    options_ = {};
    OrtCUDAProviderOptions cuda_options {};
    cuda_options.device_id = device_id;
    options_.AppendExecutionProvider_CUDA(cuda_options);

    memory_info_ = Ort::MemoryInfo("Cuda", OrtDeviceAllocator, device_id, OrtMemTypeDefault);

    LogInfo << "Using CUDA execution provider with device_id" << device_id;
}

void ONNXResMgr::use_directml(int device_id)
{
    LogInfo << VAR(device_id);

#ifdef MAA_WITH_DML

    options_ = {};
    auto status = OrtSessionOptionsAppendExecutionProvider_DML(options_, device_id);
    if (!Ort::Status(status).IsOK()) {
        LogError << "Failed to append DML execution provider with device_id" << device_id;
        return;
    }

    // 不知道为什么 DML 会 crash，感觉是 onnxruntime 的 bug，之后 onnxruntime 更新了可以再试试
    // 当前版本 onnxruntime v1.19.2 from MaaDeps. 设备 AMD RX 640
    // memory_info_ = Ort::MemoryInfo("DML", OrtDeviceAllocator, device_id, OrtMemTypeDefault);
    memory_info_ = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeDefault);

    LogInfo << "Using DML execution provider with device_id" << device_id;

#else

    LogError << "MaaFW built without DML";

#endif
}

void ONNXResMgr::use_coreml(uint32_t coreml_flag)
{
    LogInfo << VAR(coreml_flag);

#ifdef MAA_WITH_COREML

    options_ = {};
    auto status = OrtSessionOptionsAppendExecutionProvider_CoreML((OrtSessionOptions*)options_, coreml_flag);
    if (!Ort::Status(status).IsOK()) {
        LogError << "Failed to append CoreML execution provider";
    }

    // 不知道 name 是啥，先糊一个
    memory_info_ = Ort::MemoryInfo("Cpu", OrtDeviceAllocator, 0, OrtMemTypeDefault);

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

    detector_roots_.emplace_back(path);

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

std::shared_ptr<Ort::Session> ONNXResMgr::detector(const std::string& name)
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

MAA_RES_NS_END
