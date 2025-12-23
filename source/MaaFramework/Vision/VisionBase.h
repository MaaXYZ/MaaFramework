#pragma once

#include <atomic>
#include <filesystem>

#include "Common/Conf.h"
#include "MaaFramework/MaaDef.h"
#include "MaaUtils/JsonExt.hpp"
#include "MaaUtils/NoWarningCVMat.hpp"

MAA_VISION_NS_BEGIN

template <typename ResultType>
class RecoResultAPI
{
public:
    using Result = ResultType;
    using ResultsVec = std::vector<Result>;

public:
    const ResultsVec& all_results() const& { return all_results_; }

    ResultsVec&& all_results() && { return std::move(all_results_); }

    const ResultsVec& filtered_results() const& { return filtered_results_; }

    ResultsVec filtered_results() && { return std::move(filtered_results_); }

    const std::optional<Result>& best_result() const& { return best_result_; }

    std::optional<Result> best_result() && { return std::move(best_result_); }

protected:
    ResultsVec all_results_;
    ResultsVec filtered_results_;
    std::optional<Result> best_result_ = std::nullopt;
};

class VisionBase
{
public:
    using ImageEncodedBuffer = std::vector<uint8_t>;

public:
    VisionBase(cv::Mat image, std::vector<cv::Rect> rois, std::string name);

    const std::vector<ImageEncodedBuffer>& draws() const& { return draws_; }

    std::vector<ImageEncodedBuffer> draws() && { return std::move(draws_); }

protected:
    cv::Mat image_with_roi() const;

    bool next_roi();
    void reset_roi();

protected:
    cv::Mat draw_roi(const cv::Mat& base = cv::Mat()) const;
    void handle_draw(const cv::Mat& draw) const;

protected:
    const cv::Mat image_;
    const std::string name_;

    cv::Rect roi_ {};

    bool debug_draw_ = false;

private:
    void init_draw();

    std::vector<cv::Rect> rois_;
    size_t roi_index_ = 0;

    mutable std::vector<ImageEncodedBuffer> draws_;
};

MAA_VISION_NS_END
