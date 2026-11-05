#pragma once

#include <memory>

#include "Common/Conf.h"
#include "Vision/OCRSession.h"

namespace fastdeploy
{
namespace vision::ocr
{
class DBDetector;
class Recognizer;
}

namespace pipeline
{
class PPOCRv4;
}
}

MAA_RES_NS_BEGIN

// fastdeploy PP-OCR (onnxruntime backend)
class FastDeployOCRSession : public MAA_VISION_NS::OCRSession
{
public:
    // deter / ocrer may be null (rec-only model)
    FastDeployOCRSession(
        std::shared_ptr<fastdeploy::vision::ocr::DBDetector> deter,
        std::shared_ptr<fastdeploy::vision::ocr::Recognizer> recer,
        std::shared_ptr<fastdeploy::pipeline::PPOCRv4> ocrer);

    virtual ~FastDeployOCRSession() override = default;

public:
    virtual int det_max_side_len() const override;
    virtual std::optional<MAA_VISION_NS::OCRDetRecResult> det_and_rec(const cv::Mat& image) override;
    virtual std::optional<MAA_VISION_NS::OCRRecResult> rec(const cv::Mat& image) override;
    virtual std::optional<std::vector<MAA_VISION_NS::OCRRecResult>> batch_rec(const std::vector<cv::Mat>& images) override;

private:
    std::shared_ptr<fastdeploy::vision::ocr::DBDetector> deter_;
    std::shared_ptr<fastdeploy::vision::ocr::Recognizer> recer_;
    std::shared_ptr<fastdeploy::pipeline::PPOCRv4> ocrer_;
};

MAA_RES_NS_END
