#include "tasker.h"
#include "loader.h"

#include <MaaFramework/MaaAPI.h>

#include "../foundation/async.h"
#include "../foundation/classes.h"
#include "../foundation/convert.h"
#include "../foundation/utils.h"
#include "ext.h"

MAA_JS_NATIVE_CLASS_STATIC_IMPL(TaskJobImpl)

maajs::ValueType TaskJobImpl::get()
{
    return source.Value()["cached_image"];
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

MAA_JS_NATIVE_CLASS_STATIC_IMPL(TaskerImpl)

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
    if (tasker) {
        ExtContext::get(env)->taskers.del(tasker);
    }

    if (own && tasker) {
        MaaTaskerDestroy(tasker);
    }
    tasker = nullptr;
    own = false;
}

maajs::ValueType
    TaskerImpl::post_task(maajs::ValueType self, maajs::EnvType, std::string entry, maajs::OptionalParam<maajs::ValueType> param)
{
    auto id = MaaTaskerPostTask(tasker, entry.c_str(), maajs::JsonStringify(env, param.value_or(maajs::ObjectType::New(env))).c_str());
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

std::string TaskerImpl::to_string()
{
    return std::format(" handle = {:#018x}, {} ", reinterpret_cast<uintptr_t>(tasker), own ? "owned" : "rented");
}

void TaskerImpl::init_bind(maajs::ObjectType self)
{
    ExtContext::get(env)->taskers.add(tasker, self);
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
    MAA_BIND_FUNC(proto, "post_task", TaskerImpl::post_task);
    MAA_BIND_FUNC(proto, "post_stop", TaskerImpl::post_stop);
    MAA_BIND_FUNC(proto, "status", TaskerImpl::status);
    MAA_BIND_FUNC(proto, "wait", TaskerImpl::wait);
    MAA_BIND_GETTER(proto, "inited", TaskerImpl::get_inited);
    MAA_BIND_GETTER(proto, "running", TaskerImpl::get_running);
    MAA_BIND_GETTER(proto, "stopping", TaskerImpl::get_stopping);
    MAA_BIND_GETTER_SETTER(proto, "resource", TaskerImpl::get_resource, TaskerImpl::set_resource);
    MAA_BIND_GETTER_SETTER(proto, "controller", TaskerImpl::get_controller, TaskerImpl::set_controller);
    MAA_BIND_FUNC(proto, "clear_cache", TaskerImpl::clear_cache);
    // TODO: details
}

maajs::ValueType load_tasker(maajs::EnvType env)
{
    maajs::FunctionType ctor;
    maajs::NativeClass<TaskerImpl>::init(env, ctor);
    ExtContext::get(env)->taskerCtor = maajs::PersistentFunction(ctor);
    return ctor;
}
