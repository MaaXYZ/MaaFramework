#include "OCRResMgr.h"

#include <filesystem>
#include <ranges>

#include "MaaUtils/File.hpp"
#include "MaaUtils/Logger.h"
#include "MaaUtils/Platform.h"
#include "MaaUtils/StringMisc.hpp"

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

    det_option_.UseCpu();
    rec_option_.UseCpu();

    det_option_.SetCpuThreadNum(4);
    rec_option_.SetCpuThreadNum(4);
}

void OCRResMgr::use_cuda(int device_id)
{
    LogInfo << VAR(device_id);

    det_option_.UseCuda(device_id);
    rec_option_.UseCuda(device_id);
}

void OCRResMgr::use_directml(int device_id)
{
    LogInfo << VAR(device_id);

    det_option_.UseDirectML(device_id);
    rec_option_.UseDirectML(device_id);
}

void OCRResMgr::use_coreml(uint32_t coreml_flag)
{
    LogInfo << VAR(coreml_flag);

    // det_option_.UseCoreML(coreml_flag);
    // rec_option_.UseCoreML(coreml_flag);

    LogWarn << "OCR with CoreML is very poor. I don’t know the reason yet. Roll back to using CPU";

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

    roots_.emplace_back(path);

    return true;
}

void OCRResMgr::clear()
{
    LogFunc;

    roots_.clear();
    deters_.clear();
    recers_.clear();
    ocrers_.clear();
}

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
