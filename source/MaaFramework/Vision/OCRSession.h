#pragma once

#include <array>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "Common/Conf.h"
#include "MaaUtils/NoWarningCVMat.hpp"

MAA_VISION_NS_BEGIN

// Same layout as fastdeploy::vision::OCRResult (boxes / text / rec_scores).
struct OCRDetRecResult
{
    // 4 points, clockwise from top-left: x0, y0, x1, y1, x2, y2, x3, y3
    std::vector<std::array<int, 8>> boxes;
    std::vector<std::string> texts;
    std::vector<float> scores;
};

using OCRRecResult = std::pair<std::string, float>;

// OCR engine of one model (det + rec + keys), the only OCR dependency of the Vision layer.
// Not thread-safe, callers serialize the calls.
class OCRSession
{
public:
    virtual ~OCRSession() = default;

    // Keep batch clustering within the detector resize budget.
    virtual int det_max_side_len() const = 0;

    // det, then rec on each box. If det finds nothing, the whole image is recognized as one line:
    // boxes is empty and texts / scores have one element.
    virtual std::optional<OCRDetRecResult> det_and_rec(const cv::Mat& image) = 0;

    // rec only, the whole image is one line
    virtual std::optional<OCRRecResult> rec(const cv::Mat& image) = 0;

    // rec only, all images are recognized as one batch
    virtual std::optional<std::vector<OCRRecResult>> batch_rec(const std::vector<cv::Mat>& images) = 0;
};

using OCRSessionPtr = std::shared_ptr<OCRSession>;

MAA_VISION_NS_END
