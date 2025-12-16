#pragma once

#include <atomic>
#include <meojson/json.hpp>

#include "Common/Conf.h"
#include "Common/MaaTypes.h"
#include "Resource/PipelineTypes.h"
#include "Task/Context.h"
#include "Tasker/Tasker.h"
#include "Vision/OCRer.h"

MAA_TASK_NS_BEGIN

class Recognizer : public NonCopyable
{
public:
    explicit Recognizer(Tasker* tasker, Context& context, const cv::Mat& image);

public:
    RecoResult recognize(const PipelineData& pipeline_data);

    MaaRecoId get_id() const { return reco_id_; }

private:
    RecoResult direct_hit(const std::string& name);
    RecoResult template_match(const MAA_VISION_NS::TemplateMatcherParam& param, const std::string& name);
    RecoResult feature_match(const MAA_VISION_NS::FeatureMatcherParam& param, const std::string& name);
    RecoResult color_match(const MAA_VISION_NS::ColorMatcherParam& param, const std::string& name);
    RecoResult ocr(const MAA_VISION_NS::OCRerParam& param, const std::string& name);
    RecoResult nn_classify(const MAA_VISION_NS::NeuralNetworkClassifierParam& param, const std::string& name);
    RecoResult nn_detect(const MAA_VISION_NS::NeuralNetworkDetectorParam& param, const std::string& name);
    RecoResult custom_recognize(const MAA_VISION_NS::CustomRecognitionParam& param, const std::string& name);

    cv::Rect get_roi(const MAA_VISION_NS::Target& roi);
    void save_draws(const std::string& node_name, const RecoResult& result) const;

private:
    bool debug_mode() const;
    MAA_RES_NS::ResourceMgr* resource();

private:
    inline static std::atomic<MaaRecoId> s_global_reco_id = kRecoIdBase;

    Tasker* tasker_ = nullptr;
    Context& context_;
    cv::Mat image_;
    const MaaRecoId reco_id_ = ++s_global_reco_id;
};

MAA_TASK_NS_END
