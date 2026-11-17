#include "FastDeployOCRSession.h"

#include <algorithm>
#include <cmath>

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
    : context_(
          (deter && deter->runtime_option.device != fastdeploy::Device::CPU)
          || (recer && recer->runtime_option.device != fastdeploy::Device::CPU))
    , deter_(std::move(deter))
    , recer_(std::move(recer))
    , ocrer_(std::move(ocrer))
{
}

std::optional<MAA_VISION_NS::OCRDetRecResult> FastDeployOCRSession::det_and_rec(const cv::Mat& image)
{
    auto lock = context_.lock();

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

std::optional<cv::Size> FastDeployOCRSession::batch_det_shape(const cv::Size& image_size) const
{
    auto lock = context_.lock();

    if (!deter_ || !ocrer_ || image_size.width <= 0 || image_size.height <= 0 || !deter_->Initialized() || deter_->NumInputsOfRuntime() != 1
        || deter_->NumOutputsOfRuntime() != 1) {
        return std::nullopt;
    }
    const auto input = deter_->InputInfoOfRuntime(0);
    if (input.shape.size() != 4 || input.shape[0] >= 0) {
        return std::nullopt;
    }
    const auto output = deter_->OutputInfoOfRuntime(0);
    if (output.shape.size() != 4 || output.shape[0] >= 0) {
        return std::nullopt;
    }

    const auto& preprocessor = deter_->GetPreprocessor();
    if (preprocessor.GetStaticShapeInfer()) {
        const auto shape = preprocessor.GetDetImageShape();
        if (shape.size() != 3 || shape[1] <= 0 || shape[2] <= 0) {
            return std::nullopt;
        }
        return cv::Size(shape[2], shape[1]);
    }

    const int max_side_len = preprocessor.GetMaxSideLen();
    if (max_side_len <= 0) {
        return std::nullopt;
    }

    // Match DBDetectorPreprocessor::OcrDetectorGetInfo, including float truncation before rounding.
    const int width = image_size.width;
    const int height = image_size.height;
    float ratio = 1.f;
    if (std::max(width, height) > max_side_len) {
        ratio = height > width ? float(max_side_len) / float(height) : float(max_side_len) / float(width);
    }
    int resize_height = int(float(height) * ratio);
    int resize_width = int(float(width) * ratio);
    resize_height = std::max(int(std::round(float(resize_height) / 32) * 32), 32);
    resize_width = std::max(int(std::round(float(resize_width) / 32) * 32), 32);
    return cv::Size(resize_width, resize_height);
}

std::optional<std::vector<MAA_VISION_NS::OCRDetRecResult>> FastDeployOCRSession::batch_det_and_rec(const std::vector<cv::Mat>& images)
{
    auto lock = context_.lock();

    if (images.empty()) {
        return std::nullopt;
    }
    const auto shape = batch_det_shape(images.front().size());
    if (!shape) {
        return std::nullopt;
    }
    for (const auto& image : images) {
        if (image.empty() || batch_det_shape(image.size()) != shape) {
            return std::nullopt;
        }
    }

    // PPOCR batches detection across images, but keeps recognition crops and their padding inside each ROI.
    // Equal detector shapes also prevent its preprocessor from padding an ROI to another ROI's dimensions.
    std::vector<fastdeploy::vision::OCRResult> ocr_results;
    if (!ocrer_->BatchPredict(images, &ocr_results)) {
        LogWarn << "ocrer_ BatchPredict return false" << VAR(images.size());
        return std::nullopt;
    }
    if (ocr_results.size() != images.size()) {
        LogError << "Bad batch ocr result size" << VAR(images.size()) << VAR(ocr_results.size());
        return std::nullopt;
    }

    std::vector<MAA_VISION_NS::OCRDetRecResult> results;
    results.reserve(ocr_results.size());
    for (auto& result : ocr_results) {
        results.emplace_back(
            MAA_VISION_NS::OCRDetRecResult {
                .boxes = std::move(result.boxes),
                .texts = std::move(result.text),
                .scores = std::move(result.rec_scores),
            });
    }
    return results;
}

std::optional<MAA_VISION_NS::OCRRecResult> FastDeployOCRSession::rec(const cv::Mat& image)
{
    auto lock = context_.lock();

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
    auto lock = context_.lock();

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
