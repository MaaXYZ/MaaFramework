#pragma once

#include <ostream>
#include <vector>

#include <onnxruntime/onnxruntime_cxx_api.h>

#include "Utils/JsonExt.hpp"
#include "VisionBase.h"
#include "VisionTypes.h"

MAA_VISION_NS_BEGIN

class NeuralNetworkDetector : public VisionBase
{
public:
    struct Result
    {
        size_t cls_index = SIZE_MAX;
        std::string label;
        cv::Rect box {};
        double score = 0.0;

        MEO_JSONIZATION(cls_index, label, box, score);
    };
    using ResultsVec = std::vector<Result>;

public:
    void set_session(std::shared_ptr<Ort::Session> session) { session_ = std::move(session); }
    void set_param(NeuralNetworkDetectorParam param) { param_ = std::move(param); }
    std::pair<ResultsVec, size_t> analyze() const;

private:
    ResultsVec foreach_rois() const;
    ResultsVec detect(const cv::Rect& roi) const;
    void draw_result(const cv::Rect& roi, const ResultsVec& results) const;

    void filter(ResultsVec& results, const std::vector<size_t>& expected) const;
    void sort(ResultsVec& results) const;
    size_t preferred_index(const ResultsVec& results) const;

    NeuralNetworkDetectorParam param_;
    std::shared_ptr<Ort::Session> session_ = nullptr;
};

MAA_VISION_NS_END
