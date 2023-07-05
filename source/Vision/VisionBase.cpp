#include "VisionBase.h"

#include "Utils/NoWarningCV.h"

#include "MaaUtils/Logger.hpp"
#include "Utils/ImageIo.hpp"
#include "Utils/StringMisc.hpp"
#include "Utils/Time.hpp"

MAA_VISION_NS_BEGIN

VisionBase::VisionBase(InstanceInternalAPI* inst) : inst_(inst) {}

VisionBase::VisionBase(InstanceInternalAPI* inst, const cv::Mat& image) : inst_(inst)
{
    set_image(image);
}

void VisionBase::set_image(const cv::Mat& image)
{
    image_ = image;
#ifdef MAA_DEBUG
    image_draw_ = image.clone();
#endif
}

void VisionBase::set_cache(const cv::Rect& cache)
{
    cache_ = cache;
}

cv::Rect VisionBase::correct_roi(const cv::Rect& roi, const cv::Mat& image)
{
    if (image.empty()) {
        LogError << "image is empty" << VAR(image);
        return roi;
    }
    if (roi.empty()) {
        return { 0, 0, image.cols, image.rows };
    }

    cv::Rect res = roi;
    if (image.cols < res.x) {
        LogError << "roi is out of range" << VAR(image) << VAR(res);
        res.x = image.cols - res.width;
    }
    if (image.rows < res.y) {
        LogError << "roi is out of range" << VAR(image) << VAR(res);
        res.y = image.rows - res.height;
    }

    if (res.x < 0) {
        LogWarn << "roi is out of range" << VAR(image) << VAR(res);
        res.x = 0;
    }
    if (res.y < 0) {
        LogWarn << "roi is out of range" << VAR(image) << VAR(res);
        res.y = 0;
    }
    if (image.cols < res.x + res.width) {
        LogWarn << "roi is out of range" << VAR(image) << VAR(res);
        res.width = image.cols - res.x;
    }
    if (image.rows < res.y + res.height) {
        LogWarn << "roi is out of range" << VAR(image) << VAR(res);
        res.height = image.rows - res.y;
    }
    return res;
}

cv::Mat VisionBase::image_with_roi(const cv::Rect& roi) const
{
    cv::Rect roi_corrected = correct_roi(roi, image_);
    return image_(roi_corrected);
}

MAA_VISION_NS_END
