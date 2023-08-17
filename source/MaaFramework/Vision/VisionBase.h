#pragma once

#include "Conf/Conf.h"
#include "Instance/InstanceInternalAPI.hpp"
#include "Utils/NoWarningCVMat.hpp"

MAA_VISION_NS_BEGIN

class VisionBase
{
public:
    VisionBase(InstanceInternalAPI* inst);
    VisionBase(InstanceInternalAPI* inst, const cv::Mat& image);

    void set_image(const cv::Mat& image);
    void set_cache(const cv::Rect& cache);
    void set_name(std::string name);

protected:
    cv::Mat image_with_roi(const cv::Rect& roi) const;

protected:
    MAA_RES_NS::ResourceMgr* resource() const { return inst_ ? inst_->inter_resource() : nullptr; }
    InstanceStatus* status() const { return inst_ ? inst_->status() : nullptr; }

    cv::Mat draw_roi(const cv::Rect& roi) const;
    void save_image(const cv::Mat& image) const;

protected:
    cv::Mat image_;
    cv::Rect cache_;
    std::string name_;

    InstanceInternalAPI* inst_ = nullptr;

    bool debug_draw_ = false;
    bool save_draw_ = false;

private:
    void init_debug_draw();
};

MAA_VISION_NS_END
