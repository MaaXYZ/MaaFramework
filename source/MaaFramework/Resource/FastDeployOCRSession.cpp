#include "FastDeployOCRSession.h"

#ifdef _WIN32
#include "MaaUtils/SafeWindows.hpp"
#endif

MAA_SUPPRESS_CV_WARNINGS_BEGIN
#include "fastdeploy/vision/ocr/ppocr/dbdetector.h"
#include "fastdeploy/vision/ocr/ppocr/ppocr_v4.h"
#include "fastdeploy/vision/ocr/ppocr/recognizer.h"
MAA_SUPPRESS_CV_WARNINGS_END

#include "MaaUtils/Logger.h"

MAA_RES_NS_BEGIN

FastDeployOCRSession::FastDeployOCRSession(
    std::shared_ptr<fastdeploy::vision::ocr::DBDetector> deter,
    std::shared_ptr<fastdeploy::vision::ocr::Recognizer> recer,
    std::shared_ptr<fastdeploy::pipeline::PPOCRv4> ocrer)
    : deter_(std::move(deter))
    , recer_(std::move(recer))
    , ocrer_(std::move(ocrer))
{
}

int FastDeployOCRSession::det_max_side_len() const
{
    return deter_ ? deter_->GetPreprocessor().GetMaxSideLen() : 960;
}

std::optional<MAA_VISION_NS::OCRDetRecResult> FastDeployOCRSession::det_and_rec(const cv::Mat& image)
{
    if (!ocrer_) {
        LogError << "ocrer_ is null";
        return std::nullopt;
    }

    fastdeploy::vision::OCRResult ocr_result;
    if (!ocrer_->Predict(image, &ocr_result)) {
        LogWarn << "ocrer_ Predict return false" << VAR(image.size());
        return std::nullopt;
    }

    return MAA_VISION_NS::OCRDetRecResult {
        .boxes = std::move(ocr_result.boxes),
        .texts = std::move(ocr_result.text),
        .scores = std::move(ocr_result.rec_scores),
    };
}

std::optional<MAA_VISION_NS::OCRRecResult> FastDeployOCRSession::rec(const cv::Mat& image)
{
    if (!recer_) {
        LogError << "recer_ is null";
        return std::nullopt;
    }

    std::string text;
    float score = 0;
    if (!recer_->Predict(image, &text, &score)) {
        LogWarn << "recer_ Predict return false" << VAR(image.size());
        return std::nullopt;
    }
    return MAA_VISION_NS::OCRRecResult { std::move(text), score };
}

std::optional<std::vector<MAA_VISION_NS::OCRRecResult>> FastDeployOCRSession::batch_rec(const std::vector<cv::Mat>& images)
{
    if (!recer_) {
        LogError << "recer_ is null";
        return std::nullopt;
    }

    fastdeploy::vision::OCRResult ocr_result;
    if (!recer_->BatchPredict(images, &ocr_result)) {
        LogWarn << "recer_ BatchPredict return false" << VAR(images.size());
        return std::nullopt;
    }
    if (ocr_result.text.size() != images.size() || ocr_result.rec_scores.size() != images.size()) {
        LogError << "Bad ocr result size" << VAR(images.size()) << VAR(ocr_result.text) << VAR(ocr_result.rec_scores);
        return std::nullopt;
    }

    std::vector<MAA_VISION_NS::OCRRecResult> results;
    results.reserve(images.size());
    for (size_t i = 0; i < images.size(); ++i) {
        results.emplace_back(std::move(ocr_result.text[i]), ocr_result.rec_scores[i]);
    }
    return results;
}

MAA_RES_NS_END
