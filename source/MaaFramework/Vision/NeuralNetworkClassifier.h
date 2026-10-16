#pragma once

#include <ostream>
#include <vector>

#include "MaaUtils/JsonExt.hpp"
#include "InferenceSession.h"
#include "VisionBase.h"
#include "VisionTypes.h"

#include "Common/Conf.h"

MAA_VISION_NS_BEGIN

struct NeuralNetworkClassifierResult
{
    size_t cls_index = SIZE_MAX;
    std::string label;
    cv::Rect box { };
    double score = 0.0;
    std::vector<float> raw;
    std::vector<float> probs;

    MEO_JSONIZATION(cls_index, label, box, score);
};

class NeuralNetworkClassifier
    : public VisionBase
    , public RecoResultAPI<NeuralNetworkClassifierResult>
{
public:
    NeuralNetworkClassifier(
        cv::Mat image,
        std::vector<cv::Rect> rois,
        NeuralNetworkClassifierParam param,
        InferenceSessionPtr session,
        std::string name = "");

private:
    void analyze();

    Result classify() const;

    void add_results(ResultsVec results, const std::vector<std::variant<int, std::string>>& expected);
    void cherry_pick();

private:
    cv::Mat draw_result(const Result& res) const;
    void sort_(ResultsVec& results) const;

    void init_expected_indices();

private:
    const NeuralNetworkClassifierParam param_;
    InferenceSessionPtr session_ = nullptr;

    std::vector<int> expected_indices_;
};

MAA_VISION_NS_END
