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

struct NeuralNetworkDetectorResult
{
    size_t cls_index = SIZE_MAX;
    std::string label;
    cv::Rect box {};
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
        NeuralNetworkDetectorParam param,
        std::shared_ptr<Ort::Session> session,
        std::string name = "");

private:
    void analyze();

    ResultsVec detect_all_rois() const;
    ResultsVec detect(const cv::Rect& roi) const;

    void add_results(ResultsVec results, const std::vector<size_t>& expected);
    void cherry_pick();

private:
    cv::Mat draw_result(const cv::Rect& roi, const ResultsVec& results) const;
    void sort_(ResultsVec& results) const;

private:
    const NeuralNetworkDetectorParam param_;
    std::shared_ptr<Ort::Session> session_ = nullptr;
};

MAA_VISION_NS_END
