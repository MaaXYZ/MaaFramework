#pragma once

#include <filesystem>

#include "Conf/Conf.h"

#ifdef _WIN32
#include "Utils/SafeWindows.hpp"
#endif

MAA_SUPPRESS_CV_WARNINGS_BEGIN
#include "fastdeploy/vision/ocr/ppocr/dbdetector.h"
#include "fastdeploy/vision/ocr/ppocr/ppocr_v3.h"
#include "fastdeploy/vision/ocr/ppocr/recognizer.h"
MAA_SUPPRESS_CV_WARNINGS_END

#include "Utils/NoWarningCV.hpp"
#include "Utils/NonCopyable.hpp"

MAA_RES_NS_BEGIN

class OCRResMgr : public NonCopyable
{
public:
    OCRResMgr();

    void set_cpu();
    void set_cuda(int device_id);
    void set_dml(int device_id);
    void set_coreml(uint32_t coreml_flag);

    bool lazy_load(const std::filesystem::path& path, bool is_base);
    void clear();

public:
    std::shared_ptr<fastdeploy::vision::ocr::DBDetector> deter(const std::string& name);
    std::shared_ptr<fastdeploy::vision::ocr::Recognizer> recer(const std::string& name);
    std::shared_ptr<fastdeploy::pipeline::PPOCRv3> ocrer(const std::string& name);

private:
    std::shared_ptr<fastdeploy::vision::ocr::DBDetector> load_deter(const std::string& name);
    std::shared_ptr<fastdeploy::vision::ocr::Recognizer> load_recer(const std::string& name);
    std::shared_ptr<fastdeploy::pipeline::PPOCRv3> load_ocrer(const std::string& name);

    std::vector<std::filesystem::path> roots_;

    fastdeploy::RuntimeOption option_;

    std::unordered_map<std::string, std::shared_ptr<fastdeploy::vision::ocr::DBDetector>> deters_;
    std::unordered_map<std::string, std::shared_ptr<fastdeploy::vision::ocr::Recognizer>> recers_;
    std::unordered_map<std::string, std::shared_ptr<fastdeploy::pipeline::PPOCRv3>> ocrers_;
};

MAA_RES_NS_END
