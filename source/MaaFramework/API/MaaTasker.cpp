#include "MaaFramework/Instance/MaaTasker.h"

#include "Buffer/ImageBuffer.hpp"
#include "Buffer/StringBuffer.hpp"
#include "Tasker/Tasker.h"
#include "Utils/Logger.h"

MaaTasker* MaaTaskerCreate(MaaNotificationCallback callback, void* callback_arg)
{
    LogFunc << VAR_VOIDP(callback) << VAR_VOIDP(callback_arg);

    return new MAA_NS::Tasker(callback, callback_arg);
}

void MaaTaskerDestroy(MaaTasker* tasker)
{
    LogFunc << VAR_VOIDP(tasker);

    if (tasker == nullptr) {
        LogError << "handle is null";
        return;
    }

    delete tasker;
}

MaaBool MaaTaskerSetOption(MaaTasker* tasker, MaaTaskerOption key, MaaOptionValue value, MaaOptionValueSize val_size)
{
    LogFunc << VAR_VOIDP(tasker) << VAR(key) << VAR_VOIDP(value) << VAR(val_size);

    if (!tasker) {
        LogError << "handle is null";
        return false;
    }

    return tasker->set_option(key, value, val_size);
}

MaaBool MaaTaskerBindResource(MaaTasker* tasker, MaaResource* res)
{
    LogFunc << VAR_VOIDP(tasker) << VAR_VOIDP(res);

    if (!tasker || !res) {
        LogError << "handle is null";
        return false;
    }

    return tasker->bind_resource(res);
}

MaaBool MaaTaskerBindController(MaaTasker* tasker, MaaController* ctrl)
{
    LogFunc << VAR_VOIDP(tasker) << VAR_VOIDP(ctrl);

    if (!tasker || !ctrl) {
        LogError << "handle is null";
        return false;
    }

    return tasker->bind_controller(ctrl);
}

MaaBool MaaTaskerInited(MaaTasker* tasker)
{
    if (!tasker) {
        LogError << "handle is null";
        return false;
    }

    return tasker->inited();
}

MaaTaskId MaaTaskerPostPipeline(MaaTasker* tasker, const char* entry, const char* pipeline_override)
{
    LogFunc << VAR_VOIDP(tasker) << VAR(entry) << VAR(pipeline_override);

    if (!tasker) {
        LogError << "handle is null";
        return MaaInvalidId;
    }

    auto ov_opt = json::parse(pipeline_override);
    if (!ov_opt) {
        LogError << "failed to parse" << VAR(pipeline_override);
        return MaaInvalidId;
    }

    return tasker->post_pipeline(entry, *ov_opt);
}

MaaTaskId MaaTaskerPostRecognition(MaaTasker* tasker, const char* entry, const char* pipeline_override)
{
    LogFunc << VAR_VOIDP(tasker) << VAR(entry) << VAR(pipeline_override);

    if (!tasker) {
        LogError << "handle is null";
        return MaaInvalidId;
    }

    auto ov_opt = json::parse(pipeline_override);
    if (!ov_opt) {
        LogError << "failed to parse" << VAR(pipeline_override);
        return MaaInvalidId;
    }

    return tasker->post_recognition(entry, *ov_opt);
}

MaaTaskId MaaTaskerPostAction(MaaTasker* tasker, const char* entry, const char* pipeline_override)
{
    LogFunc << VAR_VOIDP(tasker) << VAR(entry) << VAR(pipeline_override);

    if (!tasker) {
        LogError << "handle is null";
        return MaaInvalidId;
    }

    auto ov_opt = json::parse(pipeline_override);
    if (!ov_opt) {
        LogError << "failed to parse" << VAR(pipeline_override);
        return MaaInvalidId;
    }

    return tasker->post_action(entry, *ov_opt);
}

MaaBool MaaTaskerOverridePipeline(MaaTasker* tasker, MaaTaskId id, const char* pipeline_override)
{
    LogFunc << VAR_VOIDP(tasker) << VAR(id) << VAR(pipeline_override);

    if (!tasker) {
        LogError << "handle is null";
        return false;
    }

    auto ov_opt = json::parse(pipeline_override);
    if (!ov_opt) {
        LogError << "failed to parse" << VAR(pipeline_override);
        return false;
    }

    return tasker->override_pipeline(id, *ov_opt);
}

MaaStatus MaaTaskerStatus(MaaTasker* tasker, MaaTaskId id)
{
    // LogFunc << VAR_VOIDP(tasker) << VAR(id);

    if (!tasker) {
        LogError << "handle is null";
        return MaaStatus_Invalid;
    }
    return tasker->status(id);
}

MaaStatus MaaTaskerWait(MaaTasker* tasker, MaaTaskId id)
{
    // LogFunc << VAR_VOIDP(tasker) << VAR(id);

    if (!tasker) {
        LogError << "handle is null";
        return MaaStatus_Invalid;
    }
    return tasker->wait(id);
}

MaaBool MaaTaskerRunning(MaaTasker* tasker)
{
    if (!tasker) {
        LogError << "handle is null";
        return false;
    }
    return tasker->running();
}

MaaBool MaaTaskerPostStop(MaaTasker* tasker)
{
    LogFunc << VAR_VOIDP(tasker);

    if (!tasker) {
        LogError << "handle is null";
        return false;
    }

    tasker->post_stop();
    return true;
}

MaaResource* MaaTaskerGetResource(MaaTasker* tasker)
{
    LogFunc << VAR_VOIDP(tasker);

    if (!tasker) {
        LogError << "handle is null";
        return nullptr;
    }

    return tasker->resource();
}

MaaController* MaaTaskerGetController(MaaTasker* tasker)
{
    LogFunc << VAR_VOIDP(tasker);

    if (!tasker) {
        LogError << "handle is null";
        return nullptr;
    }
    return tasker->controller();
}

MaaBool MaaTaskerClearCache(MaaTasker* tasker)
{
    LogFunc << VAR_VOIDP(tasker);

    if (!tasker) {
        LogError << "handle is null";
        return false;
    }

    tasker->clear_cache();
    return true;
}

#define CheckNullAndWarn(var)                        \
    if (!var) {                                      \
        LogWarn << #var << "is null, no assignment"; \
    }                                                \
    else

MaaBool MaaTaskerGetRecognitionDetail(
    MaaTasker* tasker,
    MaaRecoId reco_id,
    MaaStringBuffer* name,
    MaaBool* hit,
    MaaRect* box,
    MaaStringBuffer* detail_json,
    MaaImageBuffer* raw,
    MaaImageListBuffer* draws)
{
    if (!tasker) {
        LogError << "handle is null";
        return false;
    }

    auto result_opt = tasker->get_reco_result(reco_id);
    if (!result_opt) {
        LogError << "failed to get_reco_result" << VAR(reco_id);
        return false;
    }

    auto& result = *result_opt;

    CheckNullAndWarn(name)
    {
        name->set(result.name);
    }
    CheckNullAndWarn(hit)
    {
        *hit = result.box.has_value();
    }
    CheckNullAndWarn(box) if (result.box)
    {
        box->x = result.box->x;
        box->y = result.box->y;
        box->width = result.box->width;
        box->height = result.box->height;
    }
    CheckNullAndWarn(detail_json)
    {
        detail_json->set(result.detail.to_string());
    }
    CheckNullAndWarn(raw)
    {
        raw->set(result.raw);
    }
    CheckNullAndWarn(draws)
    {
        for (auto& d : result.draws) {
            draws->append(MAA_NS::ImageBuffer(d));
        }
    }

    return true;
}

MaaBool MaaTaskerGetNodeDetail(
    MaaTasker* tasker,
    MaaNodeId node_id,
    MaaStringBuffer* name,
    MaaRecoId* reco_id,
    MaaSize* times,
    MaaBool* completed)
{
    if (!tasker) {
        LogError << "handle is null";
        return false;
    }

    auto result_opt = tasker->get_node_detail(node_id);
    CheckNullAndWarn(!result_opt)
    {
        LogError << "failed to get_node_detail" << VAR(node_id);
        return false;
    }

    auto& result = *result_opt;

    CheckNullAndWarn(name)
    {
        name->set(result.name);
    }
    CheckNullAndWarn(reco_id)
    {
        *reco_id = result.reco_id;
    }
    CheckNullAndWarn(times)
    {
        *times = result.times;
    }
    CheckNullAndWarn(completed)
    {
        *completed = result.completed;
    }

    return true;
}

MaaBool MaaTaskerGetTaskDetail(
    MaaTasker* tasker,
    MaaTaskId task_id,
    MaaStringBuffer* entry,
    MaaNodeId* node_id_list,
    MaaSize* node_id_list_size)
{
    if (!tasker) {
        LogError << "handle is null";
        return false;
    }

    auto result_opt = tasker->get_task_detail(task_id);
    if (!result_opt) {
        LogError << "failed to get_task_detail" << VAR(task_id);
        return false;
    }

    auto& result = *result_opt;

    CheckNullAndWarn(entry)
    {
        entry->set(result.entry);
    }

    if (node_id_list_size && *node_id_list_size == 0) {
        *node_id_list_size = result.node_ids.size();
    }
    else if (node_id_list && node_id_list_size) {
        size_t size = std::min(result.node_ids.size(), static_cast<size_t>(*node_id_list_size));
        memcpy(node_id_list, result.node_ids.data(), size * sizeof(MaaNodeId));
        *node_id_list_size = size;
    }
    else {
        LogError << "failed to get task detail" << VAR(task_id) << VAR(node_id_list) << VAR(node_id_list_size);
        return false;
    }

    return true;
}

MaaBool MaaTaskerGetLatestNode(MaaTasker* tasker, const char* task_name, MaaNodeId* latest_id)
{
    if (!tasker) {
        LogError << "handle is null";
        return false;
    }

    auto result_opt = tasker->get_latest_node(task_name);
    if (!result_opt) {
        LogError << "failed to get_latest_node" << VAR(task_name);
        return false;
    }

    CheckNullAndWarn(latest_id)
    {
        *latest_id = *result_opt;
    }

    return true;
}
