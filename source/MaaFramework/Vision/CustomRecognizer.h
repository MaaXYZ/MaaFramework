#pragma once

#include <vector>

#include "Instance/InstanceInternalAPI.hpp"
#include "MaaFramework/MaaDef.h"
#include "Utils/JsonExt.hpp"
#include "VisionBase.h"
#include "VisionTypes.h"

MAA_VISION_NS_BEGIN

class CustomRecognizer : public VisionBase
{
    struct Result
    {
        cv::Rect box {};
        json::value detail;

        MEO_JSONIZATION(box, detail);
    };

    using ResultsVec = std::vector<Result>;

public:
    CustomRecognizer(
        cv::Mat image,
        CustomRecognizerParam param,
        CustomRecognizerSession session,
        InstanceInternalAPI* inst,
        std::string name = "");

    const ResultsVec& all_results() const& { return all_results_; }

    ResultsVec&& all_results() && { return std::move(all_results_); }

    const ResultsVec& filtered_results() const& { return filtered_results_; }

    ResultsVec filtered_results() && { return std::move(filtered_results_); }

    const std::optional<Result>& best_result() const& { return best_result_; }

    std::optional<Result> best_result() && { return std::move(best_result_); }

private:
    void analyze();

private:
    const CustomRecognizerParam param_;
    CustomRecognizerSession session_;
    InstanceInternalAPI* inst_ = nullptr;

private:
    ResultsVec all_results_;
    ResultsVec filtered_results_;
    std::optional<Result> best_result_ = std::nullopt;
};

MAA_VISION_NS_END
