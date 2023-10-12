#pragma once

#include "Conf/Conf.h"
#include "Utils/NoWarningCVMat.hpp"

MAA_VISION_NS_BEGIN

class VisionBase
{
public:
    void set_image(const cv::Mat& image);
    void set_name(std::string name);

protected:
    cv::Mat image_with_roi(const cv::Rect& roi) const;

protected:
    cv::Mat draw_roi(const cv::Rect& roi, const cv::Mat& base = cv::Mat()) const;
    void save_image(const cv::Mat& image) const;

protected:
    cv::Mat image_ {};
    std::string name_;

    bool debug_draw_ = false;
    bool save_draw_ = false;

private:
    void init_debug_draw();
};

MAA_VISION_NS_END
