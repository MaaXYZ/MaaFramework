#include "../include/forward.h"

#include "../include/info.h"
#include "../include/macro.h"
#include "../include/utils.h"
#include "../include/wrapper.h"

Napi::Promise context_run_task(Napi::Env env, Napi::External<MaaContext> info, std::string entry, std::string overr)
{
    auto handle = info.Data();
    auto worker = new SimpleAsyncWork<MaaTaskId, "context_run_task">(env, [handle, entry, overr]() {
        return MaaContextRunTask(handle, entry.c_str(), overr.c_str());
    });
    worker->Queue();
    return worker->Promise();
}

Napi::Promise context_run_recognition(
    Napi::Env env,
    Napi::External<MaaContext> info,
    std::string entry,
    std::string overr,
    Napi::ArrayBuffer image_buf)
{
    auto image = std::make_shared<ImageBuffer>();
    image->set(image_buf);
    auto handle = info.Data();
    auto worker = new SimpleAsyncWork<MaaRecoId, "context_run_recognition">(env, [handle, entry, overr, image]() mutable {
        return MaaContextRunRecognition(handle, entry.c_str(), overr.c_str(), *image);
    });
    worker->Queue();
    return worker->Promise();
}

Napi::Promise context_run_action(
    Napi::Env env,
    Napi::External<MaaContext> info,
    std::string entry,
    std::string overr,
    MaaRect box,
    std::string detail)
{
    auto handle = info.Data();
    auto worker = new SimpleAsyncWork<MaaNodeId, "context_run_action">(env, [handle, entry, overr, box, detail]() {
        return MaaContextRunAction(handle, entry.c_str(), overr.c_str(), &box, detail.c_str());
    });
    worker->Queue();
    return worker->Promise();
}

bool context_override_pipeline(Napi::External<MaaContext> info, std::string overr)
{
    return MaaContextOverridePipeline(info.Data(), overr.c_str());
}

bool context_override_next(Napi::External<MaaContext> info, std::string node_name, std::vector<std::string> next)
{
    StringListBuffer buffer;
    buffer.set_vector(next, [](auto str) {
        StringBuffer buf;
        buf.set(str);
        return buf;
    });
    return MaaContextOverrideNext(info.Data(), node_name.c_str(), buffer);
}

std::optional<std::string> context_get_node_data(Napi::External<MaaContext> info, std::string node_name)
{
    StringBuffer buffer;
    if (MaaContextGetNodeData(info.Data(), node_name.c_str(), buffer)) {
        return buffer.str();
    }
    else {
        return std::nullopt;
    }
}

MaaTaskId context_get_task_id(Napi::External<MaaContext> info)
{
    return MaaContextGetTaskId(info.Data());
}

std::optional<Napi::External<TaskerInfo>> context_get_tasker(ExtContextInfo* context, Napi::External<MaaContext> info)
{
    auto tasker = MaaContextGetTasker(info.Data());
    if (!tasker) {
        return std::nullopt;
    }
    auto it = context->taskers.find(tasker);
    if (it == context->taskers.end()) {
        return std::nullopt;
    }
    // 这里如果弱引用失效了会抛异常, 但是这种情况下确实不正常了, 直接抛也可以
    return it->second.Value();
}

Napi::External<MaaContext> context_clone(Napi::External<MaaContext> info)
{
    return Napi::External<MaaContext>::New(info.Env(), MaaContextClone(info.Data()));
}

void load_instance_context(Napi::Env env, Napi::Object& exports, Napi::External<ExtContextInfo> context)
{
    BIND(context_run_task);
    BIND(context_run_recognition);
    BIND(context_run_action);
    BIND(context_override_pipeline);
    BIND(context_override_next);
    BIND(context_get_node_data);
    BIND(context_get_task_id);
    BIND(context_get_tasker);
    BIND(context_clone);
}
