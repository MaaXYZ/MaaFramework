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

    MaaStatus get_status(const maajs::CallbackInfo& info)
    {
        if (!status) {
            auto currentVal = maajs::CallMember(
                info.Env(),
                source.Value(),
                "status",
                {
                    maajs::JSConvert<MaaId>::to_value(info.Env(), id),
                });
            auto current = maajs::JSConvert<MaaStatus>::from_value(info.Env(), currentVal);
            maajs::FreeValue(info.Env(), currentVal);
            status = current;
        }
        return *status;
    }

    constexpr static char name[] = "Job";

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
    return maajs::MakeNumber(info.Env(), job->get_status(info));
}

void JobImpl::init_proto(maajs::EnvType env, maajs::ConstObjectType proto)
{
    maajs::BindGetter(env, proto, "status", "Job.[getter status]", ::get_status);
}

maajs::ValueType load_job(maajs::EnvType env)
{
    maajs::ValueType ctor;
    maajs::JobNative::init(env, ctor);
    return ctor;
}
