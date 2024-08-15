#include "MaaFramework/Instance/MaaContext.h"

#include "API/MaaTypes.h"
#include "Buffer/ImageBuffer.hpp"
#include "Buffer/StringBuffer.hpp"
#include "Utils/Logger.h"

MaaTaskId MaaContextRunPipeline(MaaContext* context, const char* entry, const char* pipeline_override)
{
    LogFunc << VAR_VOIDP(context) << VAR(entry) << VAR(pipeline_override);

    if (!context) {
        LogError << "handle is null";
        return MaaInvalidId;
    }

    auto ov_opt = json::parse(pipeline_override);
    if (!ov_opt) {
        LogError << "failed to parse" << VAR(pipeline_override);
        return MaaInvalidId;
    }

    return context->run_pipeline(entry, *ov_opt);
}

MaaTaskId MaaContextRunRecognition(MaaContext* context, const char* entry, const char* pipeline_override, MaaImageBuffer* image)
{
    LogFunc << VAR_VOIDP(context) << VAR_VOIDP(image) << VAR(entry) << VAR(pipeline_override);

    if (!context || !image) {
        LogError << "handle is null";
        return MaaInvalidId;
    }

    auto ov_opt = json::parse(pipeline_override);
    if (!ov_opt) {
        LogError << "failed to parse" << VAR(pipeline_override);
        return MaaInvalidId;
    }

    const auto& mat = image->get();
    if (mat.empty()) {
        LogError << "empty image";
        return MaaInvalidId;
    }

    return context->run_recognition(entry, *ov_opt, mat);
}

MaaTaskId MaaContextRunAction(MaaContext* context, const char* entry, const char* pipeline_override, MaaRect* box, const char* reco_detail)
{
    LogFunc << VAR_VOIDP(context) << VAR(entry) << VAR(pipeline_override) << VAR(box) << VAR(reco_detail);

    if (!context) {
        LogError << "handle is null";
        return MaaInvalidId;
    }

    auto ov_opt = json::parse(pipeline_override);
    if (!ov_opt) {
        LogError << "failed to parse" << VAR(pipeline_override);
        return MaaInvalidId;
    }

    cv::Rect cvbox {};
    if (box) {
        cvbox.x = box->x;
        cvbox.y = box->y;
        cvbox.width = box->width;
        cvbox.height = box->height;
    }
    return context->run_action(entry, *ov_opt, cvbox, reco_detail);
}

MaaBool MaaContextOverridePipeline(MaaContext* context, const char* pipeline_override)
{
    LogFunc << VAR_VOIDP(context) << VAR(pipeline_override);

    if (!context) {
        LogError << "handle is null";
        return false;
    }
    auto ov_opt = json::parse(pipeline_override);
    if (!ov_opt) {
        LogError << "failed to parse" << VAR(pipeline_override);
        return false;
    }

    return context->override_pipeline(*ov_opt);
}

MaaTaskId MaaContextGetTaskId(MaaContext* context)
{
    if (!context) {
        LogError << "handle is null";
        return false;
    }

    return context->task_id();
}

MaaTasker* MaaContextGetTasker(MaaContext* context)
{
    if (!context) {
        LogError << "handle is null";
        return nullptr;
    }

    return context->tasker();
}
