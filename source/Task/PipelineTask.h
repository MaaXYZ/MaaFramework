#pragma once

#include "Resource/PipelineTypes.h"
#include "TaskBase.h"

MAA_TASK_NS_BEGIN

class PipelineTask : public TaskBase
{
public:
    using TaskBase::TaskBase;
    virtual ~PipelineTask() override = default;

    virtual bool run() override;
    virtual bool set_param(const json::value& param) override;

    virtual std::string_view type() const override { return "PipelineTask"; }

private:
    struct RecResult
    {
        cv::Rect box {};
    };

    std::optional<RecResult> recognize(MAA_PIPELINE_RES_NS::Recognition::Type type,
                                       const MAA_PIPELINE_RES_NS::Recognition::Params& param);
    std::optional<RecResult> direct_hit(const cv::Mat& image, const MAA_VISION_NS::DirectHitParams& param);
    std::optional<RecResult> template_match(const cv::Mat& image, const MAA_VISION_NS::TemplMatchingParams& param);
    std::optional<RecResult> ocr_det_and_rec(const cv::Mat& image, const MAA_VISION_NS::OcrParams& param);
    std::optional<RecResult> ocr_only_rec(const cv::Mat& image, const MAA_VISION_NS::OcrParams& param);
    std::optional<RecResult> freezes_wait(const cv::Mat& image, const MAA_VISION_NS::FreezesWaitingParams& param);

    void start_to_act(const RecResult& result);

private:
    std::string connecting_task_;
    std::string disconnected_task_;
};

MAA_TASK_NS_END
