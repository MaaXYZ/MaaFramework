#include "VisionBase.h"

#include "Utils/NoWarningCV.h"

#include "Utils/Logger.hpp"
#include "Utils/ImageIo.hpp"
#include "Utils/StringMisc.hpp"
#include "Utils/Time.hpp"
#include "VisionUtils.hpp"

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

cv::Mat VisionBase::image_with_roi(const cv::Rect& roi) const
{
    cv::Rect roi_corrected = correct_roi(roi, image_);
    return image_(roi_corrected);
}

MAA_VISION_NS_END
