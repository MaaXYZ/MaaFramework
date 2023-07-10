#pragma once

#include "Base/NonCopyable.hpp"
#include "Common/MaaConf.h"

#include <filesystem>
#include <vector>

MAA_SUPPRESS_CV_WARNINGS_BEGIN
#include "fastdeploy/vision/ocr/ppocr/dbdetector.h"
#include "fastdeploy/vision/ocr/ppocr/ppocr_v3.h"
#include "fastdeploy/vision/ocr/ppocr/recognizer.h"
MAA_SUPPRESS_CV_WARNINGS_END
#include "Utils/NoWarningCV.h"

MAA_RES_NS_BEGIN

class OCRConfig : public NonCopyable
{
public:
    OCRConfig();
    bool lazy_load(const std::filesystem::path& path, bool is_base);
    void clear();

public:
    const std::unique_ptr<fastdeploy::vision::ocr::DBDetector>& deter() const;
    const std::unique_ptr<fastdeploy::vision::ocr::Recognizer>& recer() const;
    const std::unique_ptr<fastdeploy::pipeline::PPOCRv3>& ocrer() const;

private:
    mutable std::unique_ptr<fastdeploy::vision::ocr::DBDetector> deter_ = nullptr;
    mutable std::unique_ptr<fastdeploy::vision::ocr::Recognizer> recer_ = nullptr;
    mutable std::unique_ptr<fastdeploy::pipeline::PPOCRv3> ocrer_ = nullptr;

    fastdeploy::RuntimeOption option_;

    std::filesystem::path det_model_path_;
    std::filesystem::path rec_model_path_;
    std::filesystem::path rec_label_path_;
};

MAA_RES_NS_END
