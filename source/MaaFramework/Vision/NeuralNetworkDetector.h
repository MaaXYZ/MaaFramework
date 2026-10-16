#pragma once

#include <ostream>
#include <vector>

#include "MaaUtils/JsonExt.hpp"
#include "InferenceSession.h"
#include "VisionBase.h"
#include "VisionTypes.h"

#include "Common/Conf.h"

MAA_VISION_NS_BEGIN

struct NeuralNetworkDetectorResult
{
    size_t cls_index = SIZE_MAX;
    std::string label;
    cv::Rect box { };
    double score = 0.0;

    MEO_JSONIZATION(cls_index, label, box, score);
};

class NeuralNetworkDetector
    : public VisionBase
    , public RecoResultAPI<NeuralNetworkDetectorResult>
{
public:
    NeuralNetworkDetector(
        cv::Mat image,
        std::vector<cv::Rect> rois,
        NeuralNetworkDetectorParam param,
        InferenceSessionPtr session,
        std::string name = "");

private:
    void analyze();

    ResultsVec detect(const std::vector<std::string>& labels) const;

    void
        add_results(ResultsVec results, const std::vector<std::variant<int, std::string>>& expected, const std::vector<double>& thresholds);
    void cherry_pick();

private:
    cv::Mat draw_result(const ResultsVec& results) const;
    void sort_(ResultsVec& results) const;

    void init_expected_indices(const std::vector<std::string>& labels);

private:
    std::vector<std::string> parse_labels_from_metadata() const;

private:
    const NeuralNetworkDetectorParam param_;
    InferenceSessionPtr session_ = nullptr;

    std::vector<int> expected_indices_;
};

MAA_VISION_NS_END
