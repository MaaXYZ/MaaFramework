#include "loader.h"

#include <MaaFramework/MaaAPI.h>

#include "../foundation/convert.h"
#include "classes.h"

MAA_JS_NATIVE_CLASS_STATIC_IMPL(JobNative)

struct JobImpl
{
    maajs::ObjectRefType source;
    MaaId id;

    std::optional<MaaStatus> status;

    ~JobImpl() { source.Unref(); }

    constexpr static bool hasCtor = true;

    static JobImpl* ctor([[maybe_unused]] const maajs::CallbackInfo& info)
    {
        auto job = new JobImpl;
        job->source = maajs::Persistent(info.Env(), info[0]);
        job->id = maajs::JSConvert<uint64_t>::from_value(info.Env(), info[1]);
        return job;
    }

    static void init_proto(maajs::EnvType env, maajs::ConstObjectType proto);
};

maajs::ValueType get_status(const maajs::CallbackInfo& info)
{
    auto job = maajs::JobNative::take(info.This());
#ifdef MAA_JS_IMPL_IS_NODEJS
    if (!job->status) {
        auto status = job->source.Value().As<Napi::Object>().Get("status").As<Napi::Function>().Call(
            job->source.Value(),
            {
                Napi::Number::New(info.Env(), job->id),
            });
        job->status = status.As<Napi::Number>().Int32Value();
        return status;
    }
    else {
        return Napi::Number::New(info.Env(), *job->status);
    }
#endif

#ifdef MAA_JS_IMPL_IS_QUICKJS
    if (!job->status) {
        auto func = JS_GetPropertyStr(info.context, job->source.Value(), "status");
        auto id = maajs::JSConvert<uint64_t>::to_value(info.Env(), job->id);
        auto status = JS_Call(info.context, func, job->source.Value(), 1, &id);
        JS_FreeValue(info.context, id);
        job->status = JS_VALUE_GET_INT(status);
        return status;
    }
    else {
        return JS_NewInt32(info.context, *job->status);
    }
#endif
}

void JobImpl::init_proto(maajs::EnvType env, maajs::ConstObjectType proto)
{
    maajs::BindGetter(env, proto, "status", "Job.[getter status]", get_status);
}

maajs::ValueType load_job(maajs::EnvType env)
{
    maajs::ValueType ctor;
    maajs::JobNative::init(env, ctor);
    return ctor;
}
