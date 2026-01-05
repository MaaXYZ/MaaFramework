#include "tasker.h"
#include "buffer.h"
#include "loader.h"

#include <MaaFramework/MaaAPI.h>

#include "../foundation/spec.h"
#include "callback.h"
#include "convert.h"
#include "ext.h"

maajs::ValueType TaskJobImpl::get()
{
    return maajs::CallMemberHelper<maajs::ValueType>(source.Value(), "task_detail", id);
}

TaskJobImpl* TaskJobImpl::ctor(const maajs::CallbackInfo& info)
{
    auto job = new TaskJobImpl;
    job->source = maajs::PersistentObject(info[0].As<maajs::ObjectType>());
    job->id = maajs::JSConvert<uint64_t>::from_value(info[1]);
    return job;
}

void TaskJobImpl::init_proto(maajs::ObjectType proto, maajs::FunctionType)
{
    MAA_BIND_FUNC(proto, "get", TaskJobImpl::get);
}

maajs::ValueType load_task_job(maajs::EnvType env)
{
    maajs::FunctionType ctor;
    maajs::NativeClass<TaskJobImpl>::init<JobImpl>(env, ctor, &ExtContext::get(env)->jobCtor);
    ExtContext::get(env)->taskJobCtor = maajs::PersistentFunction(ctor);
    return ctor;
}

TaskerImpl::TaskerImpl(MaaTasker* tasker, bool own)
    : tasker(tasker)
    , own(own)
{
}

TaskerImpl::~TaskerImpl()
{
    destroy();
}

void TaskerImpl::destroy()
{
    if (!tasker) {
        return;
    }

    ExtContext::get(env)->taskers.del(tasker);

    for (const auto& [id, ctx] : sinks) {
        MaaTaskerRemoveSink(tasker, id);
        delete ctx;
    }
    sinks.clear();

    for (const auto& [id, ctx] : ctxSinks) {
        MaaTaskerRemoveContextSink(tasker, id);
        delete ctx;
    }
    ctxSinks.clear();

    if (own) {
        MaaTaskerDestroy(tasker);
    }

    tasker = nullptr;
    own = false;
}

MaaSinkId TaskerImpl::add_sink(maajs::FunctionType sink)
{
    auto ctx = new maajs::CallbackContext(sink, "TaskerSink");
    auto id = MaaTaskerAddSink(tasker, TaskerSink, ctx);
    if (id != MaaInvalidId) {
        sinks[id] = ctx;
    }
    else {
        delete ctx;
    }
    return id;
}

void TaskerImpl::remove_sink(MaaSinkId id)
{
    if (auto it = sinks.find(id); it != sinks.end()) {
        MaaTaskerRemoveSink(tasker, id);
        delete it->second;
        sinks.erase(it);
    }
}

void TaskerImpl::clear_sinks()
{
    MaaTaskerClearSinks(tasker);
    for (const auto& [_, ctx] : sinks) {
        delete ctx;
    }
    sinks.clear();
}

MaaSinkId TaskerImpl::add_context_sink(maajs::FunctionType sink)
{
    auto ctx = new maajs::CallbackContext(sink, "ContextSink");
    auto id = MaaTaskerAddContextSink(tasker, ContextSink, ctx);
    if (id != MaaInvalidId) {
        ctxSinks[id] = ctx;
    }
    else {
        delete ctx;
    }
    return id;
}

void TaskerImpl::remove_context_sink(MaaSinkId id)
{
    if (auto it = ctxSinks.find(id); it != ctxSinks.end()) {
        MaaTaskerRemoveContextSink(tasker, id);
        delete it->second;
        ctxSinks.erase(it);
    }
}

void TaskerImpl::clear_context_sinks()
{
    MaaTaskerClearContextSinks(tasker);
    for (const auto& [_, ctx] : ctxSinks) {
        delete ctx;
    }
    ctxSinks.clear();
}

maajs::ValueType
    TaskerImpl::post_task(maajs::ValueType self, maajs::EnvType, std::string entry, maajs::OptionalParam<maajs::ValueType> param)
{
    auto id = MaaTaskerPostTask(tasker, entry.c_str(), maajs::JsonStringify(env, param.value_or(maajs::ObjectType::New(env))).c_str());
    return maajs::CallCtorHelper(ExtContext::get(env)->taskJobCtor, self, id);
}

maajs::ValueType TaskerImpl::post_recognition(
    maajs::ValueType self,
    maajs::EnvType,
    std::string reco_type,
    maajs::ValueType reco_param,
    maajs::ArrayBufferType image)
{
    ImageBuffer img;
    img.set(image);
    auto id = MaaTaskerPostRecognition(tasker, reco_type.c_str(), maajs::JsonStringify(env, reco_param).c_str(), img);
    return maajs::CallCtorHelper(ExtContext::get(env)->taskJobCtor, self, id);
}

maajs::ValueType TaskerImpl::post_action(
    maajs::ValueType self,
    maajs::EnvType,
    std::string action_type,
    maajs::ValueType action_param,
    MaaRect box,
    maajs::OptionalParam<std::string> reco_detail)
{
    auto id = MaaTaskerPostAction(
        tasker,
        action_type.c_str(),
        maajs::JsonStringify(env, action_param).c_str(),
        &box,
        reco_detail.value_or("").c_str());
    return maajs::CallCtorHelper(ExtContext::get(env)->taskJobCtor, self, id);
}

maajs::ValueType TaskerImpl::post_stop(maajs::ValueType self, maajs::EnvType)
{
    auto id = MaaTaskerPostStop(tasker);
    return maajs::CallCtorHelper(ExtContext::get(env)->taskJobCtor, self, id);
}

MaaStatus TaskerImpl::status(MaaTaskId id)
{
    return MaaTaskerStatus(tasker, id);
}

maajs::PromiseType TaskerImpl::wait(MaaTaskId id)
{
    auto worker = new maajs::AsyncWork<MaaStatus>(env, [handle = tasker, id]() { return MaaTaskerWait(handle, id); });
    worker->Queue();
    return worker->Promise();
}

bool TaskerImpl::get_inited()
{
    return MaaTaskerInited(tasker);
}

bool TaskerImpl::get_running()
{
    return MaaTaskerRunning(tasker);
}

bool TaskerImpl::get_stopping()
{
    return MaaTaskerStopping(tasker);
}

void TaskerImpl::set_resource(std::optional<maajs::NativeObject<ResourceImpl>> res)
{
    bool succ {};
    if (res) {
        succ = MaaTaskerBindResource(tasker, res->impl->resource);
    }
    else {
        succ = MaaTaskerBindResource(tasker, nullptr);
    }
    if (!succ) {
        throw maajs::MaaError { "Tasker set resource failed" };
    }
}

std::optional<maajs::ValueType> TaskerImpl::get_resource()
{
    auto res = MaaTaskerGetResource(tasker);
    if (!res) {
        return std::nullopt;
    }
    return ResourceImpl::locate_object(env, res);
}

void TaskerImpl::set_controller(std::optional<maajs::NativeObject<ControllerImpl>> ctrl)
{
    bool succ {};
    if (ctrl) {
        succ = MaaTaskerBindController(tasker, ctrl->impl->controller);
    }
    else {
        succ = MaaTaskerBindController(tasker, nullptr);
    }
    if (!succ) {
        throw maajs::MaaError { "Tasker set controller failed" };
    }
}

std::optional<maajs::ValueType> TaskerImpl::get_controller()
{
    auto ctrl = MaaTaskerGetController(tasker);
    if (!ctrl) {
        return std::nullopt;
    }
    return ControllerImpl::locate_object(env, ctrl);
}

void TaskerImpl::clear_cache()
{
    MaaTaskerClearCache(tasker);
}

std::optional<maajs::ValueType> TaskerImpl::recognition_detail(MaaRecoId id)
{
    StringBuffer node_name;
    StringBuffer algorithm;
    MaaBool hit = false;
    MaaRect box {};
    StringBuffer detail;
    ImageBuffer raw;
    raw.data(env);
    ImageListBuffer draws;
    if (MaaTaskerGetRecognitionDetail(tasker, id, node_name, algorithm, &hit, &box, detail, raw, draws)) {
        auto result = maajs::ObjectType::New(env);

        result["name"] = maajs::StringType::New(env, node_name.str());
        result["algorithm"] = maajs::StringType::New(env, algorithm.str());
        result["hit"] = maajs::BooleanType::New(env, hit);
        result["box"] = maajs::JSConvert<MaaRect>::to_value(env, box);
        result["detail"] = maajs::JsonParse(env, detail.str());
        result["raw"] = raw.data(env);

        auto typedDrawsArr = draws.as_vector([&](auto draw) { return draw.data(env); });
        std::vector<maajs::ValueType> drawsArr;
        for (const auto& val : typedDrawsArr) {
            drawsArr.push_back(val);
        }
        result["draws"] = maajs::MakeArray(env, drawsArr);

        return result;
    }
    else {
        return std::nullopt;
    }
}

std::optional<maajs::ValueType> TaskerImpl::action_detail(MaaActId id)
{
    StringBuffer node_name;
    StringBuffer action;
    MaaRect box {};
    MaaBool success = false;
    StringBuffer detail_json;
    if (MaaTaskerGetActionDetail(tasker, id, node_name, action, &box, &success, detail_json)) {
        auto result = maajs::ObjectType::New(env);

        result["name"] = maajs::StringType::New(env, node_name.str());
        result["action"] = maajs::StringType::New(env, action.str());
        result["box"] = maajs::JSConvert<MaaRect>::to_value(env, box);
        result["success"] = maajs::BooleanType::New(env, success);
        result["detail"] = maajs::JsonParse(env, detail_json.str());

        return result;
    }
    else {
        return std::nullopt;
    }
}

std::optional<maajs::ValueType> TaskerImpl::node_detail(MaaNodeId id)
{
    StringBuffer node_name;
    MaaRecoId reco_id = MaaInvalidId;
    MaaActId action_id = MaaInvalidId;
    MaaBool completed = false;
    if (MaaTaskerGetNodeDetail(tasker, id, node_name, &reco_id, &action_id, &completed)) {
        auto result = maajs::ObjectType::New(env);

        result["name"] = maajs::StringType::New(env, node_name.str());
        result["reco"] = reco_id == MaaInvalidId ? env.Null() : recognition_detail(reco_id).value_or(env.Null());
        result["action"] = action_id == MaaInvalidId ? env.Null() : action_detail(action_id).value_or(env.Null());
        result["completed"] = maajs::BooleanType::New(env, completed);

        return result;
    }
    else {
        return std::nullopt;
    }
}

std::optional<maajs::ValueType> TaskerImpl::task_detail(MaaTaskId id)
{
    MaaSize node_size = 0;
    if (!MaaTaskerGetTaskDetail(tasker, id, nullptr, nullptr, &node_size, nullptr)) {
        return std::nullopt;
    }
    StringBuffer entry;
    std::vector<MaaNodeId> nodes(node_size);
    MaaStatus status = MaaStatusEnum::MaaStatus_Invalid;
    if (MaaTaskerGetTaskDetail(tasker, id, entry, nodes.data(), &node_size, &status)) {
        auto result = maajs::ObjectType::New(env);

        result["entry"] = maajs::StringType::New(env, entry.str());

        std::vector<maajs::ValueType> nodesArr;
        for (auto node_id : nodes) {
            nodesArr.push_back(node_detail(node_id).value_or(env.Null()));
        }
        result["nodes"] = maajs::MakeArray(env, nodesArr);

        result["status"] = maajs::NumberType::New(env, status);

        return result;
    }
    else {
        return std::nullopt;
    }
}

std::optional<MaaNodeId> TaskerImpl::latest_node(std::string node_name)
{
    MaaNodeId id;
    if (MaaTaskerGetLatestNode(tasker, node_name.c_str(), &id)) {
        return id;
    }
    else {
        return std::nullopt;
    }
}

std::string TaskerImpl::to_string()
{
    return std::format(" handle = {:#018x}, {} ", reinterpret_cast<uintptr_t>(tasker), own ? "owned" : "rented");
}

maajs::ValueType TaskerImpl::locate_object(maajs::EnvType env, MaaTasker* tsk)
{
    if (auto obj = ExtContext::get(env)->taskers.find(tsk)) {
        return *obj;
    }
    else {
        return maajs::CallCtorHelper(ExtContext::get(env)->taskerCtor, std::to_string(reinterpret_cast<uintptr_t>(tsk)));
    }
}

void TaskerImpl::init_bind(maajs::ObjectType self)
{
    ExtContext::get(env)->taskers.add(tasker, self);
}

void TaskerImpl::gc_mark(maajs::NativeMarkerFunc marker)
{
    for (const auto& [_, ctx] : sinks) {
        marker(ctx->fn);
    }
    for (const auto& [_, ctx] : ctxSinks) {
        marker(ctx->fn);
    }
}

TaskerImpl* TaskerImpl::ctor(const maajs::CallbackInfo& info)
{
    if (info.Length() == 1) {
        try {
            MaaTasker* handle = reinterpret_cast<MaaTasker*>(std::stoull(info[0].As<maajs::StringType>().Utf8Value()));
            return new TaskerImpl { handle, false };
        }
        catch (std::exception&) {
            return nullptr;
        }
    }
    else {
        auto handle = MaaTaskerCreate();
        if (!handle) {
            return nullptr;
        }
        return new TaskerImpl { handle, true };
    }
}

void TaskerImpl::init_proto(maajs::ObjectType proto, maajs::FunctionType)
{
    MAA_BIND_FUNC(proto, "destroy", TaskerImpl::destroy);
    MAA_BIND_FUNC(proto, "add_sink", TaskerImpl::add_sink);
    MAA_BIND_FUNC(proto, "remove_sink", TaskerImpl::remove_sink);
    MAA_BIND_FUNC(proto, "clear_sinks", TaskerImpl::clear_sinks);
    MAA_BIND_FUNC(proto, "add_context_sink", TaskerImpl::add_context_sink);
    MAA_BIND_FUNC(proto, "remove_context_sink", TaskerImpl::remove_context_sink);
    MAA_BIND_FUNC(proto, "clear_context_sinks", TaskerImpl::clear_context_sinks);
    MAA_BIND_FUNC(proto, "post_task", TaskerImpl::post_task);
    MAA_BIND_FUNC(proto, "post_recognition", TaskerImpl::post_recognition);
    MAA_BIND_FUNC(proto, "post_action", TaskerImpl::post_action);
    MAA_BIND_FUNC(proto, "post_stop", TaskerImpl::post_stop);
    MAA_BIND_FUNC(proto, "status", TaskerImpl::status);
    MAA_BIND_FUNC(proto, "wait", TaskerImpl::wait);
    MAA_BIND_GETTER(proto, "inited", TaskerImpl::get_inited);
    MAA_BIND_GETTER(proto, "running", TaskerImpl::get_running);
    MAA_BIND_GETTER(proto, "stopping", TaskerImpl::get_stopping);
    MAA_BIND_GETTER_SETTER(proto, "resource", TaskerImpl::get_resource, TaskerImpl::set_resource);
    MAA_BIND_GETTER_SETTER(proto, "controller", TaskerImpl::get_controller, TaskerImpl::set_controller);
    MAA_BIND_FUNC(proto, "clear_cache", TaskerImpl::clear_cache);
    MAA_BIND_FUNC(proto, "recognition_detail", TaskerImpl::recognition_detail);
    MAA_BIND_FUNC(proto, "action_detail", TaskerImpl::action_detail);
    MAA_BIND_FUNC(proto, "node_detail", TaskerImpl::node_detail);
    MAA_BIND_FUNC(proto, "task_detail", TaskerImpl::task_detail);
    MAA_BIND_FUNC(proto, "latest_node", TaskerImpl::latest_node);
}

maajs::ValueType load_tasker(maajs::EnvType env)
{
    maajs::FunctionType ctor;
    maajs::NativeClass<TaskerImpl>::init(env, ctor);
    ExtContext::get(env)->taskerCtor = maajs::PersistentFunction(ctor);
    return ctor;
}
