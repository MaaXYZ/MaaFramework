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

    void set_image(const cv::Mat& image);
    void set_cache(const cv::Rect& cache);

#ifdef MAA_DEBUG
    const cv::Mat& get_image_draw() const { return image_draw_; }
#endif

protected:
    cv::Mat image_with_roi(const cv::Rect& roi) const;

protected:
    MAA_RES_NS::ResourceMgr* resource() const { return inst_ ? inst_->inter_resource() : nullptr; }
    InstanceStatus* status() const { return inst_ ? inst_->status() : nullptr; }

protected:
    cv::Mat image_;
#ifdef MAA_DEBUG
    cv::Mat image_draw_;
#endif
    cv::Rect cache_;

    InstanceInternalAPI* inst_ = nullptr;
};

MAA_VISION_NS_END
