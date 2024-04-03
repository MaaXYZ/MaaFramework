#pragma once

#include <atomic>
#include <filesystem>

#include "Conf/Conf.h"
#include "Utils/JsonExt.hpp"
#include "Utils/NoWarningCVMat.hpp"

MAA_VISION_NS_BEGIN

class VisionBase
{
public:
    VisionBase(cv::Mat image, std::string name);

    const std::vector<cv::Mat>& draws() const& { return draws_; }

    std::vector<cv::Mat> draws() && { return std::move(draws_); }

    uint64_t uid() const { return uid_; }

protected:
    cv::Mat image_with_roi(const cv::Rect& roi) const;

protected:
    cv::Mat draw_roi(const cv::Rect& roi, const cv::Mat& base = cv::Mat()) const;
    void handle_draw(const cv::Mat& draw);

protected:
    const cv::Mat image_;
    const std::string name_;

    bool debug_draw_ = false;
    const uint64_t uid_ = ++s_global_uid;

private:
    void init_draw();

private:
    inline static std::atomic_uint64_t s_global_uid = 0;

    std::vector<cv::Mat> draws_;
};

MAA_VISION_NS_END
