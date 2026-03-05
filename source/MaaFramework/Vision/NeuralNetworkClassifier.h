#pragma once

#include <optional>
#include <ostream>
#include <vector>

#include <onnxruntime/onnxruntime_cxx_api.h>

#include "MaaUtils/JsonExt.hpp"
#include "VisionBase.h"
#include "VisionTypes.h"

#include "Common/Conf.h"

MAA_VISION_NS_BEGIN

struct NeuralNetworkClassifierResult
{
    size_t cls_index = SIZE_MAX;
    std::string label;
    cv::Rect box {};
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
        std::shared_ptr<Ort::Session> session,
        const Ort::MemoryInfo& memory_info,
        std::string name = "");

private:
    void analyze();

    struct ModelIOInfo
    {
        std::vector<int64_t> input_shape;
        std::string input_name;
        std::string output_name;
    };

    std::optional<ModelIOInfo> load_io_info() const;
    Result classify(const ModelIOInfo& io_info) const;

    void add_results(ResultsVec results, const std::vector<int>& expected);
    void cherry_pick();

private:
    cv::Mat draw_result(const Result& res) const;
    void sort_(ResultsVec& results) const;

private:
    const NeuralNetworkClassifierParam param_;
    std::shared_ptr<Ort::Session> session_ = nullptr;
    const Ort::MemoryInfo& memory_info_;
};

MAA_VISION_NS_END
