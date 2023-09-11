#pragma once

#include <ostream>
#include <vector>

#include <onnxruntime/core/session/onnxruntime_cxx_api.h>

#include "VisionBase.h"
#include "VisionTypes.h"

MAA_VISION_NS_BEGIN

class Classifier : public VisionBase
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

        json::value to_json() const
        {
            json::value root;
            root["cls_index"] = cls_index;
            root["label"] = label;
            root["box"] = json::array({ box.x, box.y, box.width, box.height });
            root["score"] = score;
            root["raw"] = json::array(raw);
            root["probs"] = json::array(probs);
            return root;
        }
    };
    using ResultsVec = std::vector<Result>;

public:
    void set_param(ClassifierParam param) { param_ = std::move(param); }
    void set_session(std::shared_ptr<Ort::Session> session) { session_ = std::move(session); }
    ResultsVec analyze() const;

private:
    ResultsVec foreach_rois() const;
    Result classify(const cv::Rect& roi) const;
    void draw_result(const Result& res) const;

    void filter(ResultsVec& results, const std::vector<size_t>& expected) const;

    ClassifierParam param_;
    std::shared_ptr<Ort::Session> session_ = nullptr;
};

MAA_VISION_NS_END

MAA_NS_BEGIN

inline std::ostream& operator<<(std::ostream& os, const MAA_VISION_NS::Classifier::Result& res)
{
    os << res.to_json().to_string();
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const MAA_VISION_NS::Classifier::ResultsVec& resutls)
{
    json::array root;
    for (const auto& res : resutls) {
        root.emplace_back(res.to_json());
    }
    os << root.to_string();
    return os;
}

MAA_NS_END
