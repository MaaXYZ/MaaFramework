#pragma once
#pragma once

#include <ostream>
#include <vector>

#include "Utils/JsonExt.hpp"
#include "VisionBase.h"
#include "VisionTypes.h"

namespace Ort
{
struct Session;
}

MAA_VISION_NS_BEGIN

struct NeuralNetworkClassifierResult
{
    size_t cls_index = SIZE_MAX;
    std::string label;
    cv::Rect box {};
    double score = 0.0;
    std::vector<float> raw;
    std::vector<float> probs;

    MEO_JSONIZATION(cls_index, label, box, score, raw, probs);
};

class NeuralNetworkClassifier
    : public VisionBase
    , public RecoResultAPI<NeuralNetworkClassifierResult>
{
public:
    NeuralNetworkClassifier(
        cv::Mat image,
        NeuralNetworkClassifierParam param,
        std::shared_ptr<Ort::Session> session,
        std::string name = "");

private:
    void analyze();

    ResultsVec classify_all_rois() const;
    Result classify(const cv::Rect& roi) const;

    void add_results(ResultsVec results, const std::vector<size_t>& expected);
    void cherry_pick();

private:
    cv::Mat draw_result(const Result& res) const;
    void sort_(ResultsVec& results) const;

private:
    const NeuralNetworkClassifierParam param_;
    std::shared_ptr<Ort::Session> session_ = nullptr;
};

MAA_VISION_NS_END
