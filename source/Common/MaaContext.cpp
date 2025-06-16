#include "MaaFramework/Instance/MaaContext.h"

#include "Common/MaaTypes.h"
#include "Utils/Buffer/ImageBuffer.hpp"
#include "Utils/Buffer/StringBuffer.hpp"
#include "Utils/Logger.h"

MaaTaskId MaaContextRunTask(MaaContext* context, const char* entry, const char* pipeline_override)
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
    if (!ov_opt->is_object()) {
        LogError << "json is not object" << VAR(pipeline_override);
        return MaaInvalidId;
    }

    return context->run_task(entry, ov_opt->as_object());
}

MaaRecoId MaaContextRunRecognition(MaaContext* context, const char* entry, const char* pipeline_override, const MaaImageBuffer* image)
{
    LogFunc << VAR_VOIDP(context) << VAR(entry) << VAR(pipeline_override);

    if (!context || !image) {
        LogError << "handle is null";
        return MaaInvalidId;
    }

    auto ov_opt = json::parse(pipeline_override);
    if (!ov_opt) {
        LogError << "failed to parse" << VAR(pipeline_override);
        return MaaInvalidId;
    }
    if (!ov_opt->is_object()) {
        LogError << "json is not object" << VAR(pipeline_override);
        return MaaInvalidId;
    }

    const auto& mat = image->get();
    if (mat.empty()) {
        LogError << "empty image";
        return MaaInvalidId;
    }

    return context->run_recognition(entry, ov_opt->as_object(), mat);
}

MaaNodeId
    MaaContextRunAction(MaaContext* context, const char* entry, const char* pipeline_override, const MaaRect* box, const char* reco_detail)
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
    if (!ov_opt->is_object()) {
        LogError << "json is not object" << VAR(pipeline_override);
        return MaaInvalidId;
    }

    cv::Rect cvbox {};
    if (box) {
        cvbox.x = box->x;
        cvbox.y = box->y;
        cvbox.width = box->width;
        cvbox.height = box->height;
    }
    return context->run_action(entry, ov_opt->as_object(), cvbox, reco_detail);
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
    if (!ov_opt->is_object()) {
        LogError << "json is not object" << VAR(pipeline_override);
        return false;
    }

    return context->override_pipeline(ov_opt->as_object());
}

MaaBool MaaContextOverrideNext(MaaContext* context, const char* name, const MaaStringListBuffer* next_list)
{
    LogFunc << VAR_VOIDP(context) << VAR(name);

    if (!context || !next_list) {
        LogError << "handle is null";
        return false;
    }

    std::vector<std::string> next;

    size_t size = next_list->size();
    for (size_t i = 0; i < size; ++i) {
        next.emplace_back(next_list->at(i).get());
    }

    return context->override_next(name, next);
}

MaaBool MaaContextGetNodeData(MaaContext* context, const char* node_name, MaaStringBuffer* buffer)
{
    LogFunc << VAR_VOIDP(context) << VAR(node_name);

    if (!context || !buffer) {
        LogError << "handle is null";
        return false;
    }

    auto data_opt = context->get_node_data(node_name);
    if (!data_opt) {
        LogError << "failed to get node data" << VAR(node_name);
        return false;
    }

    buffer->set(data_opt->dumps());
    return true;
}

MaaTaskId MaaContextGetTaskId(const MaaContext* context)
{
    if (!context) {
        LogError << "handle is null";
        return false;
    }

    return context->task_id();
}

MaaTasker* MaaContextGetTasker(const MaaContext* context)
{
    if (!context) {
        LogError << "handle is null";
        return nullptr;
    }

    return context->tasker();
}

MaaContext* MaaContextClone(const MaaContext* context)
{
    if (!context) {
        LogError << "handle is null";
        return nullptr;
    }

    return context->clone();
}
