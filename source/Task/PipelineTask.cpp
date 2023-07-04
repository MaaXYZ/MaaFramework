#include "PipelineTask.h"

#include "MaaUtils/Logger.hpp"
#include "Resource/ResourceMgr.h"

MAA_TASK_NS_BEGIN

bool PipelineTask::run()
{
    LogFunc << VAR(task_name_);

    const auto& data = resource()->pipeline_cfg().get_data(std::string(task_name_));

    // TODO
    std::ignore = data;

    return false;
}

bool PipelineTask::set_param(const json::value& param)
{
    LogFunc << VAR(param);

    return true;
}

std::optional<cv::Rect> PipelineTask::recognize(MAA_PIPELINE_RES_NS::Recognition::Type type,
                                                const MAA_PIPELINE_RES_NS::Recognition::Params& param)
{
    using namespace MAA_PIPELINE_RES_NS::Recognition;
    using namespace MAA_VISION_NS;

    switch (type) {
    case Type::DirectHit:
        return direct_hit(std::get<DirectHitParams>(param));
    case Type::TemplateMatch:
        return template_match(std::get<TemplMatchingParams>(param));
    case Type::OcrDetAndRec:
        return ocr_det_and_rec(std::get<OcrParams>(param));
    case Type::OcrOnlyRec:
        return ocr_only_rec(std::get<OcrParams>(param));
    case Type::FreezesWait:
        return freezes_wait(std::get<FreezesWaitingParams>(param));
    default:
        LogError << "Unknown type" << VAR(static_cast<int>(type));
        return std::nullopt;
    }
}

std::optional<cv::Rect> PipelineTask::direct_hit(const MAA_VISION_NS::DirectHitParams& param)
{
    std::ignore = param;
    return std::optional<cv::Rect>();
}

std::optional<cv::Rect> PipelineTask::template_match(const MAA_VISION_NS::TemplMatchingParams& param)
{
    std::ignore = param;
    return std::optional<cv::Rect>();
}

std::optional<cv::Rect> PipelineTask::ocr_det_and_rec(const MAA_VISION_NS::OcrParams& param)
{
    std::ignore = param;
    return std::optional<cv::Rect>();
}

std::optional<cv::Rect> PipelineTask::ocr_only_rec(const MAA_VISION_NS::OcrParams& param)
{
    std::ignore = param;
    return std::optional<cv::Rect>();
}

std::optional<cv::Rect> PipelineTask::freezes_wait(const MAA_VISION_NS::FreezesWaitingParams& param)
{
    std::ignore = param;
    return std::optional<cv::Rect>();
}

MAA_TASK_NS_END
