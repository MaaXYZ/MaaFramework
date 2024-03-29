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

    const std::vector<std::filesystem::path>& draw_paths() const& { return draw_paths_; }

    std::vector<std::filesystem::path> draw_paths() && { return std::move(draw_paths_); }

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
    void init_debug_draw();
    std::filesystem::path save_image(const cv::Mat& image) const;

private:
    bool save_draw_ = false;
    std::vector<cv::Mat> draws_;
    std::vector<std::filesystem::path> draw_paths_;
    size_t preferred_index_ = SIZE_MAX;
};

MAA_VISION_NS_END
