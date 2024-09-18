#include "VisionBase.h"

#include "Utils/NoWarningCV.hpp"

#include "Global/GlobalOptionMgr.h"
#include "Utils/Logger.h"
#include "VisionUtils.hpp"

MAA_VISION_NS_BEGIN

VisionBase::VisionBase(cv::Mat image, cv::Rect roi, std::string name)
    : image_(std::move(image))
    , roi_(correct_roi(roi, image_))
    , name_(std::move(name))
{
    init_draw();
}

cv::Mat VisionBase::image_with_roi() const
{
    return image_(roi_);
}

cv::Mat VisionBase::draw_roi(const cv::Mat& base) const
{
    cv::Mat image_draw = base.empty() ? image_.clone() : base;
    const cv::Scalar color(0, 255, 0);

    cv::putText(image_draw, name_, cv::Point(5, image_.rows - 5), cv::FONT_HERSHEY_SIMPLEX, 1, color, 2);

    cv::rectangle(image_draw, roi_, color, 1);
    std::string flag = std::format("ROI: [{}, {}, {}, {}]", roi_.x, roi_.y, roi_.width, roi_.height);
    cv::putText(image_draw, flag, cv::Point(roi_.x, roi_.y - 5), cv::FONT_HERSHEY_PLAIN, 1.2, color, 1);

    return image_draw;
}

void VisionBase::handle_draw(const cv::Mat& draw) const
{
    draws_.emplace_back(draw);
}

void VisionBase::init_draw()
{
#ifdef MAA_DEBUG
    debug_draw_ = true;
#else
    const auto& option = GlobalOptionMgr::get_instance();
    debug_draw_ = option.save_draw() || option.debug_mode();
#endif
}

MAA_VISION_NS_END
