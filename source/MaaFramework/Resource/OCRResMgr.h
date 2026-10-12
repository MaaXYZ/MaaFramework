#pragma once

#include <filesystem>
#include <mutex>
#include <unordered_map>

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
#include "Vision/OCRSession.h"

MAA_RES_NS_BEGIN

class OCRResMgr : public NonCopyable
{
public:
    OCRResMgr();

    void use_cpu();
    void use_cuda(int device_id);
    void use_directml(int device_id);
    void use_coreml(uint32_t coreml_flag);
    void use_webgpu(int device_id);
    // returns false if MaaFW is built without NCNN
    bool use_ncnn();

    bool lazy_load(const std::filesystem::path& path);
    void clear();

public:
    // det + rec + keys of `name`, null if rec / keys are missing or fail to load
    MAA_VISION_NS::OCRSessionPtr session(const std::string& name);

private:
    inline static const std::filesystem::path kDetModelFilename = "det.onnx";
    inline static const std::filesystem::path kRecModelFilename = "rec.onnx";
    inline static const std::filesystem::path kKeysFilename = "keys.txt";

    MAA_VISION_NS::OCRSessionPtr load_session(const std::string& name);
#ifdef MAA_WITH_NCNN
    MAA_VISION_NS::OCRSessionPtr load_ncnn_session(const std::string& name);
#endif

    std::shared_ptr<fastdeploy::vision::ocr::DBDetector> deter(const std::string& name);
    std::shared_ptr<fastdeploy::vision::ocr::Recognizer> recer(const std::string& name);
    std::shared_ptr<fastdeploy::pipeline::PPOCRv4> ocrer(const std::string& name);

    std::shared_ptr<fastdeploy::vision::ocr::DBDetector> load_deter(const std::string& name);
    std::shared_ptr<fastdeploy::vision::ocr::Recognizer> load_recer(const std::string& name);
    std::shared_ptr<fastdeploy::pipeline::PPOCRv4> load_ocrer(const std::string& name);

    std::vector<std::filesystem::path> roots_;

    fastdeploy::RuntimeOption det_option_;
    fastdeploy::RuntimeOption rec_option_;

    std::unordered_map<std::string, std::shared_ptr<fastdeploy::vision::ocr::DBDetector>> deters_;
    std::unordered_map<std::string, std::shared_ptr<fastdeploy::vision::ocr::Recognizer>> recers_;
    std::unordered_map<std::string, std::shared_ptr<fastdeploy::pipeline::PPOCRv4>> ocrers_;

    bool use_ncnn_ = false;
#ifdef MAA_WITH_NCNN
    int ncnn_threads_ = 0;
#endif

    std::mutex session_mutex_;
    std::unordered_map<std::string, MAA_VISION_NS::OCRSessionPtr> sessions_;
};

MAA_RES_NS_END
