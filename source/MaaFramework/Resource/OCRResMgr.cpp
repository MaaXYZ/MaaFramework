#include "OCRResMgr.h"

#include <filesystem>
#include <ranges>

#include "MaaUtils/File.hpp"
#include "MaaUtils/Logger.h"
#include "MaaUtils/Platform.h"
#include "MaaUtils/StringMisc.hpp"
#include "FastDeployOCRSession.h"
#include "NcnnOCRSession.h"
#include "NcnnSession.h"

MAA_RES_NS_BEGIN

OCRResMgr::OCRResMgr()
{
    LogFunc;

    det_option_.UseOrtBackend();
    rec_option_.UseOrtBackend();
}

void OCRResMgr::use_cpu()
{
    LogInfo;

    use_ncnn_ = false;

    det_option_.UseCpu();
    rec_option_.UseCpu();

    det_option_.SetCpuThreadNum(4);
    rec_option_.SetCpuThreadNum(4);
}

void OCRResMgr::use_cuda(int device_id)
{
    LogInfo << VAR(device_id);

    use_ncnn_ = false;

    det_option_.UseCuda(device_id);
    rec_option_.UseCuda(device_id);
}

void OCRResMgr::use_directml(int device_id)
{
    LogInfo << VAR(device_id);

    use_ncnn_ = false;

    det_option_.UseDirectML(device_id);
    rec_option_.UseDirectML(device_id);
}

void OCRResMgr::use_webgpu(int device_id)
{
    LogInfo << VAR(device_id);

    use_ncnn_ = false;

    det_option_.UseWebGPU(device_id);
    rec_option_.UseWebGPU(device_id);
}

bool OCRResMgr::use_ncnn()
{
    LogInfo;

#ifdef MAA_WITH_NCNN
    use_ncnn_ = true;
    ncnn_threads_ = NcnnSession::default_num_threads();
    LogInfo << "Using NCNN CPU for OCR" << VAR(ncnn_threads_);
    return true;
#else
    LogError << "MaaFW built without NCNN";
    return false;
#endif
}

void OCRResMgr::use_coreml(uint32_t coreml_flag)
{
    LogWarn << "CoreML is deprecated and no longer shipped. OCR falls back to CPU; use WebGPU instead" << VAR(coreml_flag);

    use_cpu();
}

bool OCRResMgr::lazy_load(const std::filesystem::path& path)
{
    LogFunc << VAR(path);

    if (!std::filesystem::is_directory(path)) {
        LogError << "path is not directory" << VAR(path);
        return false;
    }

    bool has_file = false;
    for (auto& entry : std::filesystem::recursive_directory_iterator(path)) {
        auto& entry_path = entry.path();
        if (entry.is_directory()) {
            continue;
        }

        auto filename = entry_path.filename();
        if (filename == kDetModelFilename || filename == kRecModelFilename || filename == kKeysFilename) {
            has_file = true;
            break;
        }
    }

    if (!has_file) {
        LogError << "model file not found" << VAR(path) << VAR(kDetModelFilename) << VAR(kRecModelFilename) << VAR(kKeysFilename);
        return false;
    }

    std::unique_lock lock(session_mutex_);
    roots_.emplace_back(path);

    return true;
}

void OCRResMgr::clear()
{
    LogFunc;

    std::unique_lock lock(session_mutex_);

    roots_.clear();
    deters_.clear();
    recers_.clear();
    ocrers_.clear();
    sessions_.clear();
}

MAA_VISION_NS::OCRSessionPtr OCRResMgr::session(const std::string& name)
{
    // multiple taskers may share one resource, guard lookup + load + insert
    std::unique_lock lock(session_mutex_);

    if (auto iter = sessions_.find(name); iter != sessions_.end()) {
        return iter->second;
    }

    auto session = load_session(name);
    if (session) {
        sessions_.emplace(name, session);
    }
    return session;
}

MAA_VISION_NS::OCRSessionPtr OCRResMgr::load_session(const std::string& name)
{
#ifdef MAA_WITH_NCNN
    if (use_ncnn_) {
        // bound to the packaged ncnn models: never fall back to onnxruntime
        return load_ncnn_session(name);
    }
#endif

    auto rec = recer(name);
    if (!rec) {
        LogError << "Failed to load rec" << VAR(name);
        return nullptr;
    }
    // det is optional (only_rec)
    auto det = deter(name);
    auto ocr = det ? ocrer(name) : nullptr;
    return std::make_shared<FastDeployOCRSession>(std::move(det), std::move(rec), std::move(ocr));
}

#ifdef MAA_WITH_NCNN
MAA_VISION_NS::OCRSessionPtr OCRResMgr::load_ncnn_session(const std::string& name)
{
    LogFunc << VAR(name) << VAR(roots_);

    for (const auto& root : roots_ | std::views::reverse) {
        const auto dir = root / MAA_NS::path(name);
        // any model file makes this root the override, an incomplete one fails instead of falling to lower roots
        const bool has_ncnn = std::filesystem::exists(dir / NcnnOCRSession::kRecParamFilename)
                              || std::filesystem::exists(dir / NcnnOCRSession::kRecBinFilename)
                              || std::filesystem::exists(dir / NcnnOCRSession::kRecSidecarFilename)
                              || std::filesystem::exists(dir / NcnnOCRSession::kDetParamFilename)
                              || std::filesystem::exists(dir / NcnnOCRSession::kDetBinFilename)
                              || std::filesystem::exists(dir / NcnnOCRSession::kDetSidecarFilename);
        const bool has_onnx = std::filesystem::exists(dir / kRecModelFilename) || std::filesystem::exists(dir / kDetModelFilename);
        if (!has_ncnn && !has_onnx) {
            continue;
        }
        if (!has_ncnn) {
            LogError << "ncnn ocr model is missing, expect rec.ncnn.param/.bin (and det.ncnn.param/.bin) in" << VAR(dir);
            return nullptr;
        }
        auto session = NcnnOCRSession::create(dir, ncnn_threads_);
        if (!session) {
            LogError << "Failed to load ncnn ocr model" << VAR(dir);
        }
        return session;
    }

    LogError << "ocr model not found" << VAR(name) << VAR(roots_);
    return nullptr;
}
#endif

std::shared_ptr<fastdeploy::vision::ocr::DBDetector> OCRResMgr::deter(const std::string& name)
{
    if (auto iter = deters_.find(name); iter != deters_.end()) {
        return iter->second;
    }

    auto deter = load_deter(name);
    if (deter) {
        deters_.emplace(name, deter);
    }

    return deter;
}

std::shared_ptr<fastdeploy::vision::ocr::Recognizer> OCRResMgr::recer(const std::string& name)
{
    if (auto iter = recers_.find(name); iter != recers_.end()) {
        return iter->second;
    }

    auto recer = load_recer(name);
    if (recer) {
        recers_.emplace(name, recer);
    }

    return recer;
}

std::shared_ptr<fastdeploy::pipeline::PPOCRv4> OCRResMgr::ocrer(const std::string& name)
{
    if (auto iter = ocrers_.find(name); iter != ocrers_.end()) {
        return iter->second;
    }

    auto ocrer = load_ocrer(name);
    if (ocrer) {
        ocrers_.emplace(name, ocrer);
    }

    return ocrer;
}

std::shared_ptr<fastdeploy::vision::ocr::DBDetector> OCRResMgr::load_deter(const std::string& name)
{
    using namespace path_literals;

    LogFunc << VAR(name) << VAR(roots_);

    for (const auto& root : roots_ | std::views::reverse) {
        const auto dir = root / MAA_NS::path(name);
        const auto model_path = dir / kDetModelFilename;
        if (!std::filesystem::exists(model_path)) {
            continue;
        }
        LogDebug << VAR(model_path);

        auto det = std::make_shared<fastdeploy::vision::ocr::DBDetector>(
            path_to_utf8_string(model_path),
            std::string(),
            det_option_,
            fastdeploy::ModelFormat::ONNX);
        if (!det || !det->Initialized()) {
            LogError << "Failed to load DBDetector:" << VAR(name) << VAR(det) << VAR(det->Initialized());
            return nullptr;
        }
        return det;
    }

    return nullptr;
}

std::shared_ptr<fastdeploy::vision::ocr::Recognizer> OCRResMgr::load_recer(const std::string& name)
{
    using namespace path_literals;

    LogFunc << VAR(name) << VAR(roots_);

    for (const auto& root : roots_ | std::views::reverse) {
        const auto dir = root / MAA_NS::path(name);
        const auto model_path = dir / kRecModelFilename;
        const auto label_path = dir / kKeysFilename;
        if (!std::filesystem::exists(model_path) || !std::filesystem::exists(label_path)) {
            continue;
        }
        LogDebug << VAR(model_path);

        auto rec = std::make_shared<fastdeploy::vision::ocr::Recognizer>(
            path_to_utf8_string(model_path),
            std::string(),
            path_to_utf8_string(label_path),
            rec_option_,
            fastdeploy::ModelFormat::ONNX);
        if (!rec || !rec->Initialized()) {
            LogError << "Failed to load Recognizer:" << VAR(name) << VAR(rec) << VAR(rec->Initialized());
            return nullptr;
        }
        return rec;
    }

    return nullptr;
}

std::shared_ptr<fastdeploy::pipeline::PPOCRv4> OCRResMgr::load_ocrer(const std::string& name)
{
    using namespace path_literals;

    LogFunc << VAR(name) << VAR(roots_);

    auto det = deter(name);
    auto rec = recer(name);
    if (!det || !rec) {
        LogError << "Failed to load det or rec:" << VAR(name) << VAR(det) << VAR(rec);
        return nullptr;
    }

    auto ocr = std::make_shared<fastdeploy::pipeline::PPOCRv4>(det.get(), rec.get());

    if (!ocr || !ocr->Initialized()) {
        LogError << "Failed to load PPOCRv4:" << VAR(name) << VAR(ocr) << VAR(ocr->Initialized());
        return nullptr;
    }
    return ocr;
}

MAA_RES_NS_END
