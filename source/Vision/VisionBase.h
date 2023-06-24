#pragma once

#include "Common/MaaConf.h"
#include "Instance/InstanceInternalAPI.hpp"
#include "Utils/NoWarningCVMat.h"

MAA_VISION_NS_BEGIN

class VisionBase
{
public:
    VisionBase(InstanceInternalAPI* inst);
    VisionBase(InstanceInternalAPI* inst, const cv::Mat& image);
    VisionBase(InstanceInternalAPI* inst, const cv::Mat& image, const cv::Rect& roi);

    void set_image(const cv::Mat& image);
    void set_roi(const cv::Rect& roi);

#ifdef MAA_DEBUG
    const cv::Mat& get_image_draw() const { return image_draw_; }
#endif

protected:
    static cv::Rect correct_roi(const cv::Rect& roi, const cv::Mat& image);

protected:
    MAA_RES_NS::ResourceMgr* resource() const { return inst_->resource(); }

protected:
    cv::Mat image_;
#ifdef MAA_DEBUG
    cv::Mat image_draw_;
#endif
    cv::Rect roi_;

    InstanceInternalAPI* inst_ = nullptr;
};

MAA_VISION_NS_END
