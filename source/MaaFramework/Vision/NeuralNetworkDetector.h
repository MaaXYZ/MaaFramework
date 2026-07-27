#pragma once

#include <ostream>
#include <vector>

#include "MaaUtils/JsonExt.hpp"
#include "NeuralNetwork/ModelPackage.h"
#include "VisionBase.h"
#include "VisionTypes.h"

#include <onnxruntime/onnxruntime_cxx_api.h>

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
        std::shared_ptr<const NeuralNetwork::ModelPackage> package,
        std::string name = "");

    const std::string& error() const { return error_; }

private:
    struct CandidateResult
    {
        size_t cls_index = SIZE_MAX;
        std::string label;
        cv::Rect2f box { };
        double score = 0.0;
    };

    using CandidateResults = std::vector<CandidateResult>;

    void analyze();

    bool detect(const std::vector<std::string>& labels, double score_floor, CandidateResults& results, std::string& error) const;

    void finalize(CandidateResults all_results, CandidateResults filtered_results);

private:
    cv::Mat draw_result(const ResultsVec& results) const;
    void sort_(CandidateResults& results) const;
    Result to_result(CandidateResult result) const;

private:
    const NeuralNetworkDetectorParam param_;
    std::shared_ptr<const NeuralNetwork::ModelPackage> package_;
    std::string error_;
};

MAA_VISION_NS_END
