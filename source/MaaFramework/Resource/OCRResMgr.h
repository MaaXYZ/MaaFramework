#pragma once

#include <filesystem>

#include "Common/Conf.h"

#ifdef _WIN32
#include "MaaUtils/SafeWindows.hpp"
#endif

MAA_SUPPRESS_CV_WARNINGS_BEGIN
#include "fastdeploy/vision/ocr/ppocr/dbdetector.h"
#include "fastdeploy/vision/ocr/ppocr/ppocr_v4.h"
#include "fastdeploy/vision/ocr/ppocr/recognizer.h"
MAA_SUPPRESS_CV_WARNINGS_END

#include "MaaUtils/NoWarningCV.hpp"
#include "MaaUtils/NonCopyable.hpp"

MAA_RES_NS_BEGIN

class OCRResMgr : public NonCopyable
{
public:
    OCRResMgr();

    void use_cpu();
    void use_cuda(int device_id);
    void use_directml(int device_id);
    void use_coreml(uint32_t coreml_flag);

    bool lazy_load(const std::filesystem::path& path);
    void clear();

public:
    std::shared_ptr<fastdeploy::vision::ocr::DBDetector> deter(const std::string& name);
    std::shared_ptr<fastdeploy::vision::ocr::Recognizer> recer(const std::string& name);
    std::shared_ptr<fastdeploy::pipeline::PPOCRv4> ocrer(const std::string& name);

private:
    inline static const std::filesystem::path kDetModelFilename = "det.onnx";
    inline static const std::filesystem::path kRecModelFilename = "rec.onnx";
    inline static const std::filesystem::path kKeysFilename = "keys.txt";

    std::shared_ptr<fastdeploy::vision::ocr::DBDetector> load_deter(const std::string& name);
    std::shared_ptr<fastdeploy::vision::ocr::Recognizer> load_recer(const std::string& name);
    std::shared_ptr<fastdeploy::pipeline::PPOCRv4> load_ocrer(const std::string& name);

    std::vector<std::filesystem::path> roots_;

    fastdeploy::RuntimeOption det_option_;
    fastdeploy::RuntimeOption rec_option_;

    std::unordered_map<std::string, std::shared_ptr<fastdeploy::vision::ocr::DBDetector>> deters_;
    std::unordered_map<std::string, std::shared_ptr<fastdeploy::vision::ocr::Recognizer>> recers_;
    std::unordered_map<std::string, std::shared_ptr<fastdeploy::pipeline::PPOCRv4>> ocrers_;
};

MAA_RES_NS_END
