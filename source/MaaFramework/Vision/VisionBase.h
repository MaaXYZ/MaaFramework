#pragma once

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

    size_t preferred_index() const { return preferred_index_; }

protected:
    cv::Mat image_with_roi(const cv::Rect& roi) const;

protected:
    cv::Mat draw_roi(const cv::Rect& roi, const cv::Mat& base = cv::Mat()) const;
    void handle_draw(const cv::Mat& draw);
    void handle_index(size_t total, int index);

protected:
    const cv::Mat image_;
    const std::string name_;

    bool debug_draw_ = false;

private:
    void init_draw();

private:
    std::vector<cv::Mat> draws_;
    size_t preferred_index_ = SIZE_MAX;
};

MAA_VISION_NS_END
