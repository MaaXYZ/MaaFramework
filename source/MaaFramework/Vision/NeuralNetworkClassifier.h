#pragma once
#pragma once

#include <ostream>
#include <vector>

#include <onnxruntime/onnxruntime_cxx_api.h>

#include "Utils/JsonExt.hpp"
#include "VisionBase.h"
#include "VisionTypes.h"

MAA_VISION_NS_BEGIN

class NeuralNetworkClassifier : public VisionBase
{
public:
    struct Result
    {
        size_t cls_index = SIZE_MAX;
        std::string label;
        cv::Rect box {};
        double score = 0.0;
        std::vector<float> raw;
        std::vector<float> probs;

        MEO_JSONIZATION(cls_index, label, box, score, raw, probs);
    };

    using ResultsVec = std::vector<Result>;

public:
    NeuralNetworkClassifier(
        cv::Mat image,
        NeuralNetworkClassifierParam param,
        std::shared_ptr<Ort::Session> session,
        std::string name = "");

    const ResultsVec& all_results() const& { return all_results_; }

    ResultsVec&& all_results() && { return std::move(all_results_); }

    const ResultsVec& filtered_results() const& { return filtered_results_; }

    ResultsVec filtered_results() && { return std::move(filtered_results_); }

    const std::optional<Result>& best_result() const& { return best_result_; }

    std::optional<Result> best_result() && { return std::move(best_result_); }

private:
    void analyze();

    ResultsVec classify_all_rois();
    Result classify(const cv::Rect& roi);

    void add_results(ResultsVec results, const std::vector<size_t>& expected);
    void cherry_pick();

private:
    cv::Mat draw_result(const Result& res) const;
    void sort_(ResultsVec& results) const;

private:
    const NeuralNetworkClassifierParam param_;
    std::shared_ptr<Ort::Session> session_ = nullptr;

private:
    ResultsVec all_results_;
    ResultsVec filtered_results_;
    std::optional<Result> best_result_ = std::nullopt;
};

MAA_VISION_NS_END
