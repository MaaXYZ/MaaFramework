#pragma once

#include <atomic>
#include <filesystem>

#include "Conf/Conf.h"
#include "MaaFramework/MaaDef.h"
#include "Utils/JsonExt.hpp"
#include "Utils/NoWarningCVMat.hpp"

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
    VisionBase(cv::Mat image, cv::Rect roi, std::string name);

    const std::vector<cv::Mat>& draws() const& { return draws_; }

    std::vector<cv::Mat> draws() && { return std::move(draws_); }

    MaaRecoId uid() const { return uid_; }

    static MaaRecoId generate_uid() { return ++s_global_uid; }
protected:
    cv::Mat image_with_roi() const;

protected:
    cv::Mat draw_roi(const cv::Mat& base = cv::Mat()) const;
    void handle_draw(const cv::Mat& draw) const;

protected:
    const cv::Mat image_;
    const cv::Rect roi_;
    const std::string name_;

    bool debug_draw_ = false;
    const MaaRecoId uid_ = ++s_global_uid;

private:
    void init_draw();

    mutable std::vector<cv::Mat> draws_;

private:
    inline static std::atomic<MaaRecoId> s_global_uid = 300'000'000;
};

MAA_VISION_NS_END
