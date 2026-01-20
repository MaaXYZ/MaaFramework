#include "MaaFramework/Instance/MaaTasker.h"

#include "Common/MaaTypes.h"
#include "MaaUtils/Buffer/ImageBuffer.hpp"
#include "MaaUtils/Buffer/StringBuffer.hpp"
#include "MaaUtils/Logger.h"

MaaSinkId MaaTaskerAddSink(MaaTasker* tasker, MaaEventCallback sink, void* trans_arg)
{
    LogInfo << VAR_VOIDP(tasker) << VAR_VOIDP(sink) << VAR_VOIDP(trans_arg);

    if (!tasker) {
        LogError << "handle is null";
        return MaaInvalidId;
    }

    return tasker->add_sink(sink, trans_arg);
}

void MaaTaskerRemoveSink(MaaTasker* tasker, MaaSinkId sink_id)
{
    LogInfo << VAR_VOIDP(tasker) << VAR(sink_id);

    if (!tasker) {
        LogError << "handle is null";
        return;
    }

    tasker->remove_sink(sink_id);
}

void MaaTaskerClearSinks(MaaTasker* tasker)
{
    LogInfo << VAR_VOIDP(tasker);

    if (!tasker) {
        LogError << "handle is null";
        return;
    }

    tasker->clear_sinks();
}

MaaSinkId MaaTaskerAddContextSink(MaaTasker* tasker, MaaEventCallback sink, void* trans_arg)
{
    LogInfo << VAR_VOIDP(tasker) << VAR_VOIDP(sink) << VAR_VOIDP(trans_arg);

    if (!tasker) {
        LogError << "handle is null";
        return MaaInvalidId;
    }

    return tasker->add_context_sink(sink, trans_arg);
}

void MaaTaskerRemoveContextSink(MaaTasker* tasker, MaaSinkId sink_id)
{
    LogInfo << VAR_VOIDP(tasker) << VAR(sink_id);

    if (!tasker) {
        LogError << "handle is null";
        return;
    }

    tasker->remove_context_sink(sink_id);
}

void MaaTaskerClearContextSinks(MaaTasker* tasker)
{
    LogInfo << VAR_VOIDP(tasker);

    if (!tasker) {
        LogError << "handle is null";
        return;
    }

    tasker->clear_context_sinks();
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

MaaBool MaaTaskerInited(const MaaTasker* tasker)
{
    if (!tasker) {
        LogError << "handle is null";
        return false;
    }

    return tasker->inited();
}

MaaTaskId MaaTaskerPostTask(MaaTasker* tasker, const char* entry, const char* pipeline_override)
{
    LogFunc << VAR_VOIDP(tasker) << VAR(entry) << VAR(pipeline_override);

    if (!tasker) {
        LogError << "handle is null";
        return MaaInvalidId;
    }

    if (!entry) {
        LogError << "entry is null";
        return MaaInvalidId;
    }

    if (!pipeline_override) {
        LogError << "pipeline_override is null";
        return MaaInvalidId;
    }

    auto ov_opt = json::parse(pipeline_override);
    if (!ov_opt) {
        LogError << "failed to parse" << VAR(pipeline_override);
        return MaaInvalidId;
    }

    return tasker->post_task(entry, *ov_opt);
}

MaaTaskId MaaTaskerPostRecognition(MaaTasker* tasker, const char* reco_type, const char* reco_param, const MaaImageBuffer* image)
{
    LogFunc << VAR_VOIDP(tasker) << VAR(reco_type) << VAR(reco_param);

    if (!tasker) {
        LogError << "handle is null";
        return MaaInvalidId;
    }

    if (!reco_type) {
        LogError << "reco_type is null";
        return MaaInvalidId;
    }

    if (!reco_param) {
        LogError << "reco_param is null";
        return MaaInvalidId;
    }

    if (!image) {
        LogError << "image is null";
        return MaaInvalidId;
    }

    auto param_opt = json::parse(reco_param);
    if (!param_opt) {
        LogError << "failed to parse" << VAR(reco_param);
        return MaaInvalidId;
    }

    return tasker->post_recognition(reco_type, *param_opt, image->get());
}

MaaTaskId
    MaaTaskerPostAction(MaaTasker* tasker, const char* action_type, const char* action_param, const MaaRect* box, const char* reco_detail)
{
    LogFunc << VAR_VOIDP(tasker) << VAR(action_type) << VAR(action_param) << VAR(reco_detail);

    if (!tasker) {
        LogError << "handle is null";
        return MaaInvalidId;
    }

    if (!action_type) {
        LogError << "action_type is null";
        return MaaInvalidId;
    }

    if (!action_param) {
        LogError << "action_param is null";
        return MaaInvalidId;
    }

    if (!box) {
        LogError << "box is null";
        return MaaInvalidId;
    }

    if (!reco_detail) {
        LogError << "reco_detail is null";
        return MaaInvalidId;
    }

    auto param_opt = json::parse(action_param);
    if (!param_opt) {
        LogError << "failed to parse" << VAR(action_param);
        return MaaInvalidId;
    }

    cv::Rect cv_box { box->x, box->y, box->width, box->height };
    return tasker->post_action(action_type, *param_opt, cv_box, reco_detail);
}

MaaStatus MaaTaskerStatus(const MaaTasker* tasker, MaaTaskId id)
{
    // LogFunc << VAR_VOIDP(tasker) << VAR(id);

    if (!tasker) {
        LogError << "handle is null";
        return MaaStatus_Invalid;
    }
    return tasker->status(id);
}

MaaStatus MaaTaskerWait(const MaaTasker* tasker, MaaTaskId id)
{
    // LogFunc << VAR_VOIDP(tasker) << VAR(id);

    if (!tasker) {
        LogError << "handle is null";
        return MaaStatus_Invalid;
    }
    return tasker->wait(id);
}

MaaBool MaaTaskerRunning(const MaaTasker* tasker)
{
    if (!tasker) {
        LogError << "handle is null";
        return false;
    }
    return tasker->running();
}

MaaTaskId MaaTaskerPostStop(MaaTasker* tasker)
{
    LogFunc << VAR_VOIDP(tasker);

    if (!tasker) {
        LogError << "handle is null";
        return MaaInvalidId;
    }

    return tasker->post_stop();
}

MaaBool MaaTaskerStopping(const MaaTasker* tasker)
{
    if (!tasker) {
        LogError << "handle is null";
        return false;
    }

    return tasker->stopping();
}

MaaResource* MaaTaskerGetResource(const MaaTasker* tasker)
{
    if (!tasker) {
        LogError << "handle is null";
        return nullptr;
    }

    return tasker->resource();
}

MaaController* MaaTaskerGetController(const MaaTasker* tasker)
{
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

MaaBool MaaTaskerOverridePipeline(MaaTasker* tasker, MaaTaskId task_id, const char* pipeline_override)
{
    LogFunc << VAR_VOIDP(tasker) << VAR(task_id) << VAR(pipeline_override);

    if (!tasker) {
        LogError << "handle is null";
        return false;
    }

    if (!pipeline_override) {
        LogError << "pipeline_override is null";
        return false;
    }

    auto ov_opt = json::parse(pipeline_override);
    if (!ov_opt) {
        LogError << "failed to parse" << VAR(pipeline_override);
        return false;
    }

    return tasker->override_pipeline(task_id, *ov_opt);
}

#define CheckNullAndWarn(var)                        \
    if (!var) {                                      \
        LogWarn << #var << "is null, no assignment"; \
    }                                                \
    else

MaaBool MaaTaskerGetRecognitionDetail(
    const MaaTasker* tasker,
    MaaRecoId reco_id,
    MaaStringBuffer* name,
    MaaStringBuffer* algorithm,
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
    CheckNullAndWarn(algorithm)
    {
        algorithm->set(result.algorithm);
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

MaaBool MaaTaskerGetActionDetail(
    const MaaTasker* tasker,
    MaaActId action_id,
    MaaStringBuffer* name,
    MaaStringBuffer* action,
    MaaRect* box,
    MaaBool* success,
    MaaStringBuffer* detail_json)
{
    if (!tasker) {
        LogError << "handle is null";
        return false;
    }

    auto result_opt = tasker->get_action_result(action_id);
    if (!result_opt) {
        LogError << "failed to get_action_result" << VAR(action_id);
        return false;
    }

    auto& result = *result_opt;

    CheckNullAndWarn(name)
    {
        name->set(result.name);
    }
    CheckNullAndWarn(action)
    {
        action->set(result.action);
    }
    CheckNullAndWarn(box)
    {
        box->x = result.box.x;
        box->y = result.box.y;
        box->width = result.box.width;
        box->height = result.box.height;
    }
    CheckNullAndWarn(success)
    {
        *success = result.success;
    }
    CheckNullAndWarn(detail_json)
    {
        detail_json->set(result.detail.to_string());
    }

    return true;
}

MaaBool MaaTaskerGetNodeDetail(
    const MaaTasker* tasker,
    MaaNodeId node_id,
    MaaStringBuffer* name,
    MaaRecoId* reco_id,
    MaaActId* action_id,
    MaaBool* completed)
{
    if (!tasker) {
        LogError << "handle is null";
        return false;
    }

    auto result_opt = tasker->get_node_detail(node_id);
    if (!result_opt) {
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
    CheckNullAndWarn(action_id)
    {
        *action_id = result.action_id;
    }
    CheckNullAndWarn(completed)
    {
        *completed = result.completed;
    }

    return true;
}

MaaBool MaaTaskerGetTaskDetail(
    const MaaTasker* tasker,
    MaaTaskId task_id,
    MaaStringBuffer* entry,
    MaaNodeId* node_id_list,
    MaaSize* node_id_list_size,
    MaaStatus* status)
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

    CheckNullAndWarn(status)
    {
        *status = result.status;
    }

    return true;
}

MaaBool MaaTaskerGetLatestNode(const MaaTasker* tasker, const char* node_name, MaaNodeId* latest_id)
{
    if (!tasker) {
        LogError << "handle is null";
        return false;
    }

    if (!node_name) {
        LogError << "node_name is null";
        return false;
    }

    auto result_opt = tasker->get_latest_node(node_name);
    if (!result_opt) {
        LogError << "failed to get_latest_node" << VAR(node_name);
        return false;
    }

    CheckNullAndWarn(latest_id)
    {
        *latest_id = *result_opt;
    }

    return true;
}
