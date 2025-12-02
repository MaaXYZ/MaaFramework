#pragma once

#include <memory>
#include <meojson/json.hpp>

#include "Common/Conf.h"
#include "Common/MaaTypes.h"
#include "PipelineTypes.h"
#include "Task/Context.h"
#include "Tasker/Tasker.h"
#include "Vision/OCRer.h"

MAA_RES_NS_BEGIN

class RecoJob
{
public:
    RecoJob(MaaRecoId reco_id, const PipelineData& pipeline_data, std::shared_ptr<MAA_TASK_NS::Context> context, const cv::Mat& image);

    MaaRecoId reco_id() const { return reco_id_; }

    void run(MaaResource* res);

private:
    MAA_TASK_NS::RecoResult direct_hit(const std::string& name);
    MAA_TASK_NS::RecoResult template_match(const MAA_VISION_NS::TemplateMatcherParam& param, const std::string& name);
    MAA_TASK_NS::RecoResult feature_match(const MAA_VISION_NS::FeatureMatcherParam& param, const std::string& name);
    MAA_TASK_NS::RecoResult color_match(const MAA_VISION_NS::ColorMatcherParam& param, const std::string& name);
    MAA_TASK_NS::RecoResult ocr(const MAA_VISION_NS::OCRerParam& param, const std::string& name);
    MAA_TASK_NS::RecoResult nn_classify(const MAA_VISION_NS::NeuralNetworkClassifierParam& param, const std::string& name);
    MAA_TASK_NS::RecoResult nn_detect(const MAA_VISION_NS::NeuralNetworkDetectorParam& param, const std::string& name);
    MAA_TASK_NS::RecoResult custom_recognize(const MAA_VISION_NS::CustomRecognitionParam& param, const std::string& name);

    cv::Rect get_roi(const MAA_VISION_NS::Target& roi);
    void save_draws(const std::string& node_name, const MAA_TASK_NS::RecoResult& result) const;

    bool debug_mode() const;

private:
    MaaRecoId reco_id_ = MaaInvalidId;
    PipelineData pipeline_data_;
    std::shared_ptr<MAA_TASK_NS::Context> context_;
    cv::Mat image_;
    MaaResource* resource_ = nullptr;
};

MAA_RES_NS_END

