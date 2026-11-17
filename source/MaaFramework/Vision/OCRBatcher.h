#pragma once

#include <span>
#include <vector>

#include "OCRTypes.h"

MAA_VISION_NS_BEGIN

class OCRBatcher
{
public:
    static constexpr size_t kBatchSize = 4;

    OCRBatcher(cv::Mat image, const std::vector<cv::Rect>& rois, OCRSessionPtr session, OCRCache& cache);

private:
    void analyze(const std::vector<cv::Rect>& rois);
    void predict(std::span<const cv::Rect> rois);

    const cv::Mat image_;
    const OCRSessionPtr session_;
    OCRCache& cache_;
};

MAA_VISION_NS_END
