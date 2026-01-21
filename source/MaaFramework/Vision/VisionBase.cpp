#include "VisionBase.h"

#include "MaaUtils/NoWarningCV.hpp"

#include "Global/OptionMgr.h"
#include "MaaUtils/Logger.h"
#include "VisionUtils.hpp"

MAA_VISION_NS_BEGIN

VisionBase::VisionBase(cv::Mat image, std::vector<cv::Rect> rois, std::string name)
    : image_(std::move(image))
    , name_(std::move(name))
    , rois_(correct_rois(std::move(rois), image_))
{
    init_draw();
}

cv::Mat VisionBase::image_with_roi() const
{
    return image_(roi_);
}

bool VisionBase::next_roi()
{
    if (roi_index_ >= rois_.size()) {
        return false;
    }
    roi_ = rois_[roi_index_++];
    return true;
}

void VisionBase::reset_roi()
{
    roi_index_ = 0;
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
    const auto& option = MAA_GLOBAL_NS::OptionMgr::get_instance();
    int quality = option.draw_quality();

    ImageEncodedBuffer jpg;
    if (!cv::imencode(".jpg", draw, jpg, { cv::IMWRITE_JPEG_QUALITY, quality })) {
        LogError << "Failed to encode draw image" << VAR(name_);
        return;
    }

    draws_.emplace_back(std::move(jpg));
}

void VisionBase::init_draw()
{
#ifdef MAA_DEBUG
    debug_draw_ = true;
#else
    const auto& option = MAA_GLOBAL_NS::OptionMgr::get_instance();
    debug_draw_ = option.save_draw() || option.debug_mode();
#endif
}

MAA_VISION_NS_END
