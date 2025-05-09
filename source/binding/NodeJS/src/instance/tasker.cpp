#include "../include/forward.h"

#include "../include/cb.h"
#include "../include/info.h"
#include "../include/macro.h"
#include "../include/utils.h"
#include "../include/wrapper.h"

std::optional<Napi::External<TaskerInfo>> tasker_create(Napi::Env env, ExtContextInfo* context, std::optional<Napi::Function> callback)
{
    MaaNotificationCallback cb = nullptr;
    CallbackContext* ctx = nullptr;
    MaaTasker* handle = nullptr;

    if (callback) {
        cb = NotificationCallback;
        ctx = new CallbackContext { env, callback.value(), "NotificationCallback" };
    }

    handle = MaaTaskerCreate(cb, ctx);

    if (handle) {
        auto info = Napi::External<TaskerInfo>::New(env, new TaskerInfo { { handle }, ctx }, &DeleteFinalizer<TaskerInfo*>);
        context->taskers[handle] = Napi::Weak(info);
        return info;
    }
    else {
        delete ctx;
        return std::nullopt;
    }
}

void tasker_destroy(ExtContextInfo* context, Napi::External<TaskerInfo> info)
{
    context->taskers.erase(info.Data()->handle);
    info.Data()->dispose();
}

bool tasker_bind_resource(Napi::External<TaskerInfo> info, std::optional<Napi::External<ResourceInfo>> res_info)
{
    if (res_info) {
        if (MaaTaskerBindResource(info.Data()->handle, res_info->Data()->handle)) {
            info.Data()->resource = Napi::Persistent(res_info.value());
            return true;
        }
        else {
            return false;
        }
    }
    else {
        if (MaaTaskerBindResource(info.Data()->handle, nullptr)) {
            info.Data()->resource.Reset();
            return true;
        }
        else {
            return false;
        }
    }
}

bool tasker_bind_controller(Napi::External<TaskerInfo> info, std::optional<Napi::External<ControllerInfo>> ctrl_info)
{
    if (ctrl_info) {
        if (MaaTaskerBindController(info.Data()->handle, ctrl_info->Data()->handle)) {
            info.Data()->controller = Napi::Persistent(ctrl_info.value());
            return true;
        }
        else {
            return false;
        }
    }
    else {
        if (MaaTaskerBindResource(info.Data()->handle, nullptr)) {
            info.Data()->controller.Reset();
            return true;
        }
        else {
            return false;
        }
    }
}

bool tasker_inited(Napi::External<TaskerInfo> info)
{
    return MaaTaskerInited(info.Data()->handle);
}

MaaTaskId tasker_post_task(Napi::External<TaskerInfo> info, std::string entry, std::string overr)
{
    return MaaTaskerPostTask(info.Data()->handle, entry.c_str(), overr.c_str());
}

MaaStatus tasker_status(Napi::External<TaskerInfo> info, MaaTaskId id)
{
    return MaaTaskerStatus(info.Data()->handle, id);
}

Napi::Promise tasker_wait(Napi::Env env, Napi::External<TaskerInfo> info, MaaTaskId id)
{
    auto handle = info.Data()->handle;
    auto worker = new SimpleAsyncWork<MaaStatus, "tasker_wait">(env, [handle, id]() { return MaaTaskerWait(handle, id); });
    worker->Queue();
    return worker->Promise();
}

bool tasker_running(Napi::External<TaskerInfo> info)
{
    return MaaTaskerRunning(info.Data()->handle);
}

MaaTaskId tasker_post_stop(Napi::External<TaskerInfo> info)
{
    return MaaTaskerPostStop(info.Data()->handle);
}

bool tasker_stopping(Napi::External<TaskerInfo> info)
{
    return MaaTaskerStopping(info.Data()->handle);
}

// TODO: 考虑下要不要下面两个也做成全局记录的, 而非基于ref的
std::optional<Napi::External<ResourceInfo>> tasker_get_resource(Napi::External<TaskerInfo> info)
{
    if (info.Data()->resource.IsEmpty()) {
        return std::nullopt;
    }
    else {
        return info.Data()->resource.Value();
    }
}

std::optional<Napi::External<ControllerInfo>> tasker_get_controller(Napi::External<TaskerInfo> info)
{
    if (info.Data()->controller.IsEmpty()) {
        return std::nullopt;
    }
    else {
        return info.Data()->controller.Value();
    }
}

bool tasker_clear_cache(Napi::External<TaskerInfo> info)
{
    return MaaTaskerClearCache(info.Data()->handle);
}

std::optional<std::tuple<std::string, std::string, bool, MaaRect, std::string, Napi::ArrayBuffer, std::vector<Napi::ArrayBuffer>>>
    tasker_get_recognition_detail(Napi::Env env, Napi::External<TaskerInfo> info, MaaRecoId id)
{
    StringBuffer name;
    StringBuffer algorithm;
    MaaBool hit = false;
    MaaRect box {};
    StringBuffer detail;
    ImageBuffer raw;
    ImageListBuffer draws;
    if (MaaTaskerGetRecognitionDetail(info.Data()->handle, id, name, algorithm, &hit, &box, detail, raw, draws)) {
        return std::make_tuple(name.str(), algorithm.str(), !!hit, box, detail.str(), raw.data(env), draws.as_vector([&](auto draw) {
            return draw.data(env);
        }));
    }
    else {
        return std::nullopt;
    }
}

std::optional<std::tuple<std::string, MaaRecoId, bool>> tasker_get_node_detail(Napi::External<TaskerInfo> info, MaaNodeId id)
{
    StringBuffer name;
    MaaRecoId reco_id = MaaInvalidId;
    MaaBool completed = false;
    if (MaaTaskerGetNodeDetail(info.Data()->handle, id, name, &reco_id, &completed)) {
        return std::make_tuple(name.str(), reco_id, completed);
    }
    else {
        return std::nullopt;
    }
}

std::optional<std::tuple<std::string, std::vector<MaaNodeId>, MaaStatus>>
    tasker_get_task_detail(Napi::External<TaskerInfo> info, MaaTaskId id)
{
    MaaSize node_size = 0;
    if (!MaaTaskerGetTaskDetail(info.Data()->handle, id, nullptr, nullptr, &node_size, nullptr)) {
        return std::nullopt;
    }
    StringBuffer entry;
    std::vector<MaaNodeId> nodes(node_size);
    MaaStatus status = MaaStatusEnum::MaaStatus_Invalid;
    if (MaaTaskerGetTaskDetail(info.Data()->handle, id, entry, nodes.data(), &node_size, &status)) {
        return std::make_tuple(entry.str(), nodes, status);
    }
    else {
        return std::nullopt;
    }
}

std::optional<MaaNodeId> tasker_get_latest_node(Napi::External<TaskerInfo> info, std::string name)
{
    MaaNodeId latest_id = MaaInvalidId;
    if (MaaTaskerGetLatestNode(info.Data()->handle, name.c_str(), &latest_id)) {
        return latest_id;
    }
    else {
        return std::nullopt;
    }
}

void load_instance_tasker(Napi::Env env, Napi::Object& exports, Napi::External<ExtContextInfo> context)
{
    BIND(tasker_create);
    BIND(tasker_destroy);
    BIND(tasker_bind_resource);
    BIND(tasker_bind_controller);
    BIND(tasker_inited);
    BIND(tasker_post_task);
    BIND(tasker_status);
    BIND(tasker_wait);
    BIND(tasker_running);
    BIND(tasker_post_stop);
    BIND(tasker_stopping);
    BIND(tasker_get_resource);
    BIND(tasker_get_controller);
    BIND(tasker_clear_cache);
    BIND(tasker_get_recognition_detail);
    BIND(tasker_get_node_detail);
    BIND(tasker_get_task_detail);
    BIND(tasker_get_latest_node);
}
