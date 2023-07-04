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
    std::optional<cv::Rect> recognize(MAA_PIPELINE_RES_NS::Recognition::Type type,
                                      const MAA_PIPELINE_RES_NS::Recognition::Params& param);
    std::optional<cv::Rect> direct_hit(const MAA_VISION_NS::DirectHitParams& param);
    std::optional<cv::Rect> template_match(const MAA_VISION_NS::TemplMatchingParams& param);
    std::optional<cv::Rect> ocr_det_and_rec(const MAA_VISION_NS::OcrParams& param);
    std::optional<cv::Rect> ocr_only_rec(const MAA_VISION_NS::OcrParams& param);
    std::optional<cv::Rect> freezes_wait(const MAA_VISION_NS::FreezesWaitingParams& param);

private:
    std::string connecting_task_;
    std::string disconnected_task_;
};

MAA_TASK_NS_END
