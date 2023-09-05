#include "OCRResMgr.h"

#include <filesystem>

#include "Utils/Demangle.hpp"
#include "Utils/File.hpp"
#include "Utils/Logger.h"
#include "Utils/Platform.h"
#include "Utils/Ranges.hpp"
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

    using namespace path_literals;
    const auto det_model_file = path / "det.onnx"_path;

    if (std::filesystem::exists(det_model_file) && det_model_path_ != det_model_file) {
        det_model_path_ = det_model_file;
        deter_ = nullptr;
    }

    const auto rec_model_file = path / "rec.onnx"_path;
    const auto rec_label_file = path / "keys.txt"_path;

    if (std::filesystem::exists(rec_model_file) && rec_model_path_ != rec_model_file) {
        rec_model_path_ = rec_model_file;
        recer_ = nullptr;
    }
    if (std::filesystem::exists(rec_label_file) && rec_model_path_ != rec_label_file) {
        rec_label_path_ = rec_label_file;
        recer_ = nullptr;
    }

    if (deter_ && recer_) {
        ocrer_ = std::make_unique<fastdeploy::pipeline::PPOCRv3>(deter_.get(), recer_.get());
    }

    LogInfo << VAR(det_model_path_) << VAR(rec_model_path_) << VAR(rec_label_path_);

    bool path_ready = !det_model_path_.empty() && !rec_model_path_.empty() && !rec_label_path_.empty();
    if (!path_ready) {
        return false;
    }

#ifdef MAA_DEBUG
    if (!deter() || !recer() || !ocrer()) {
        LogError << "failed to load OCR config";
        return false;
    }
#endif

    return path_ready;
}

void OCRResMgr::clear()
{
    LogFunc;

    ocrer_ = nullptr;
    recer_ = nullptr;
    deter_ = nullptr;
}

const std::unique_ptr<fastdeploy::vision::ocr::DBDetector>& OCRResMgr::deter() const
{
    if (deter_) {
        return deter_;
    }

    LogFunc << "Load Detector" << VAR(det_model_path_);

    auto det_model = read_file<std::string>(det_model_path_);
    auto det_option = option_;

    det_option.SetModelBuffer(det_model.data(), det_model.size(), nullptr, 0, fastdeploy::ModelFormat::ONNX);

    deter_ = std::make_unique<fastdeploy::vision::ocr::DBDetector>("dummy.onnx", std::string(), det_option,
                                                                   fastdeploy::ModelFormat::ONNX);
    if (!deter_ || !deter_->Initialized()) {
        LogError << "failed to init deter" << VAR(deter_);
        static decltype(deter_) empty;
        return empty;
    }
    return deter_;
}

const std::unique_ptr<fastdeploy::vision::ocr::Recognizer>& OCRResMgr::recer() const
{
    if (recer_) {
        return recer_;
    }

    LogFunc << "Load Recognizer" << VAR(rec_model_path_) << VAR(rec_label_path_);

    auto rec_model = read_file<std::string>(rec_model_path_);
    std::string rec_label = read_file<std::string>(rec_label_path_);

    auto rec_option = option_;
    rec_option.SetModelBuffer(rec_model.data(), rec_model.size(), nullptr, 0, fastdeploy::ModelFormat::ONNX);

    recer_ = std::make_unique<fastdeploy::vision::ocr::Recognizer>("dummy.onnx", std::string(), rec_label, rec_option,
                                                                   fastdeploy::ModelFormat::ONNX);
    if (!recer_ || !recer_->Initialized()) {
        LogError << "failed to init recer" << VAR(recer_);
        static decltype(recer_) empty;
        return empty;
    }
    return recer_;
}

const std::unique_ptr<fastdeploy::pipeline::PPOCRv3>& OCRResMgr::ocrer() const
{
    if (ocrer_) {
        return ocrer_;
    }

    LogFunc << "Load OCRer" << VAR(deter()) << VAR(recer());

    ocrer_ = std::make_unique<fastdeploy::pipeline::PPOCRv3>(deter().get(), recer().get());

    if (!ocrer_ || !ocrer_->Initialized()) {
        LogError << "failed to init ocrer_" << VAR(ocrer_);
        static decltype(ocrer_) empty;
        return empty;
    }
    return ocrer_;
}

MAA_RES_NS_END
