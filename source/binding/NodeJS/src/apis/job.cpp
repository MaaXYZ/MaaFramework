#include "loader.h"

#include <iostream>

#include <MaaFramework/MaaAPI.h>

#include "../foundation/classes.h"
#include "../foundation/macros.h"
#include "ext.h"

struct JobImpl : public maajs::NativeClassBase
{
    maajs::ObjectRefType source;
    MaaId id;

    std::optional<MaaStatus> status;

    ~JobImpl() { source.Unref(); }

    MaaStatus get_status(maajs::EnvType env)
    {
        if (!status) {
            status = maajs::CallMemberHelper<MaaStatus>(env, source.Value(), "status", id);
        }
        return *status;
    }

    constexpr static char name[] = "Job";

    static JobImpl* ctor([[maybe_unused]] const maajs::CallbackInfo& info)
    {
        auto job = new JobImpl;
        job->source = maajs::PersistentObject(info.Env(), info[0]);
        job->id = maajs::JSConvert<uint64_t>::from_value(info.Env(), info[1]);
        return job;
    }

    static void init_proto(maajs::EnvType env, maajs::ConstObjectType proto);

    void gc_mark(std::function<void(maajs::ConstValueType)> marker) override { marker(source.Value()); }
};

MAA_JS_NATIVE_CLASS_STATIC_IMPL(JobImpl)

void JobImpl::init_proto(maajs::EnvType env, maajs::ConstObjectType proto)
{
    MAA_BIND_GETTER(env, proto, "status", JobImpl::get_status);
}

maajs::ValueType load_job(maajs::EnvType env)
{
    maajs::ValueType ctor;
    maajs::NativeClass<JobImpl>::init(env, ctor);
    ExtContext::get(env)->jobCtor = maajs::PersistentFunction(env, ctor);
    return ctor;
}
