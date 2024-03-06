#include "OCRResMgr.h"

#include <filesystem>
#include <ranges>

#include "Utils/File.hpp"
#include "Utils/Logger.h"
#include "Utils/Platform.h"
#include "Utils/StringMisc.hpp"

MAA_RES_NS_BEGIN

OCRResMgr::OCRResMgr()
{
    option_.UseOrtBackend();
}

bool OCRResMgr::lazy_load(const std::filesystem::path& path, bool is_base)
{
    LogFunc << VAR(path) << VAR(is_base);

    if (is_base) {
        clear();
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

std::shared_ptr<fastdeploy::vision::ocr::DBDetector> OCRResMgr::deter(const std::string& name) const
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

std::shared_ptr<fastdeploy::vision::ocr::Recognizer> OCRResMgr::recer(const std::string& name) const
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

std::shared_ptr<fastdeploy::pipeline::PPOCRv3> OCRResMgr::ocrer(const std::string& name) const
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

std::shared_ptr<fastdeploy::vision::ocr::DBDetector>
    OCRResMgr::load_deter(const std::string& name) const
{
    using namespace path_literals;

    LogFunc << VAR(name) << VAR(roots_);

    for (const auto& root : roots_ | std::views::reverse) {
        const auto dir = root / MAA_NS::path(name);
        const auto model_path = dir / "det.onnx"_path;
        if (!std::filesystem::exists(model_path)) {
            continue;
        }

        LogTrace << VAR(model_path);

        auto model = read_file<std::string>(model_path);

        auto option = option_;
        option
            .SetModelBuffer(model.data(), model.size(), nullptr, 0, fastdeploy::ModelFormat::ONNX);

        auto det = std::make_shared<fastdeploy::vision::ocr::DBDetector>(
            "dummy.onnx",
            std::string(),
            option,
            fastdeploy::ModelFormat::ONNX);
        if (!det || !det->Initialized()) {
            LogError << "Failed to load DBDetector:" << VAR(name) << VAR(det)
                     << VAR(det->Initialized());
            return nullptr;
        }
        return det;
    }

    return nullptr;
}

std::shared_ptr<fastdeploy::vision::ocr::Recognizer>
    OCRResMgr::load_recer(const std::string& name) const
{
    using namespace path_literals;

    LogFunc << VAR(name) << VAR(roots_);

    for (const auto& root : roots_ | std::views::reverse) {
        const auto dir = root / MAA_NS::path(name);
        const auto model_path = dir / "rec.onnx"_path;
        const auto label_path = dir / "keys.txt"_path;
        if (!std::filesystem::exists(model_path) || !std::filesystem::exists(label_path)) {
            continue;
        }

        LogTrace << VAR(model_path);

        auto model = read_file<std::string>(model_path);
        auto label = read_file<std::string>(label_path);

        auto option = option_;
        option
            .SetModelBuffer(model.data(), model.size(), nullptr, 0, fastdeploy::ModelFormat::ONNX);

        auto rec = std::make_shared<fastdeploy::vision::ocr::Recognizer>(
            "dummy.onnx",
            std::string(),
            label,
            option,
            fastdeploy::ModelFormat::ONNX);
        if (!rec || !rec->Initialized()) {
            LogError << "Failed to load Recognizer:" << VAR(name) << VAR(rec)
                     << VAR(rec->Initialized());
            return nullptr;
        }
        return rec;
    }

    return nullptr;
}

std::shared_ptr<fastdeploy::pipeline::PPOCRv3> OCRResMgr::load_ocrer(const std::string& name) const
{
    using namespace path_literals;

    LogFunc << VAR(name) << VAR(roots_);

    auto det = deter(name);
    auto rec = recer(name);
    if (!det || !rec) {
        LogError << "Failed to load det or rec:" << VAR(name) << VAR(det) << VAR(rec);
        return nullptr;
    }

    auto ocr = std::make_shared<fastdeploy::pipeline::PPOCRv3>(det.get(), rec.get());

    if (!ocr || !ocr->Initialized()) {
        LogError << "Failed to load PPOCRv3:" << VAR(name) << VAR(ocr) << VAR(ocr->Initialized());
        return nullptr;
    }
    return ocr;
}

MAA_RES_NS_END