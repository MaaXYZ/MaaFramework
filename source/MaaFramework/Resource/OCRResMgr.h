#pragma once

#include <filesystem>

#include "Conf/Conf.h"

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
    bool lazy_load(const std::filesystem::path& path, bool is_base);
    void clear();

public:
    std::shared_ptr<fastdeploy::vision::ocr::DBDetector> deter(const std::string& name) const;
    std::shared_ptr<fastdeploy::vision::ocr::Recognizer> recer(const std::string& name) const;
    std::shared_ptr<fastdeploy::pipeline::PPOCRv3> ocrer(const std::string& name) const;

private:
    std::shared_ptr<fastdeploy::vision::ocr::DBDetector> load_deter(const std::string& name) const;
    std::shared_ptr<fastdeploy::vision::ocr::Recognizer> load_recer(const std::string& name) const;
    std::shared_ptr<fastdeploy::pipeline::PPOCRv3> load_ocrer(const std::string& name) const;

    std::vector<std::filesystem::path> roots_;

    fastdeploy::RuntimeOption option_;

    mutable std::unordered_map<std::string, std::shared_ptr<fastdeploy::vision::ocr::DBDetector>>
        deters_;
    mutable std::unordered_map<std::string, std::shared_ptr<fastdeploy::vision::ocr::Recognizer>>
        recers_;
    mutable std::unordered_map<std::string, std::shared_ptr<fastdeploy::pipeline::PPOCRv3>> ocrers_;
};

MAA_RES_NS_END
