#include "loader.h"

#include <cassert>
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

    JobImpl() { std::cout << "job create!" << std::endl; }

    ~JobImpl()
    {
        std::cout << "job destroy!" << std::endl;
        source.Unref();
    }

    maajs::ValueType get_source(maajs::EnvType env) { return maajs::DupValue(env, source.Value()); }

    MaaId get_id() { return id; }

    MaaStatus get_status(maajs::EnvType env)
    {
        if (!status) {
            status = maajs::CallMemberHelper<MaaStatus>(env, source.Value(), "status", id);
        }
        return *status;
    }

    maajs::ValueType wait(maajs::ConstValueType self, maajs::EnvType env)
    {
        auto pro = maajs::CallMemberHelper<maajs::ValueType>(env, source.Value(), "wait", id);

        auto selfPtr = std::make_shared<maajs::ObjectRefType>(maajs::PersistentObject(env, self, true));
        auto newPro = maajs::CallMemberHelper<maajs::ValueType>(
            env,
            maajs::ValueToObject(pro),
            "then",
            maajs::MakeFunction(
                env,
                "",
                1,
                [selfPtr](const maajs::CallbackInfo& info) { return maajs::DupValue(info.Env(), selfPtr->Value()); },
                [selfPtr](auto marker) { marker(selfPtr->Value()); }));
        maajs::FreeValue(env, pro);

        std::vector<std::string> forwards = { "status", "done", "succeeded", "failed" };
        for (auto key : forwards) {
            auto newProPtr = std::make_shared<maajs::ObjectRefType>(maajs::PersistentObject(env, newPro, true));

            maajs::BindGetter(
                env,
                maajs::ValueToObject(newPro),
                key.c_str(),
                "job_forward",
                [key, newProPtr](const maajs::CallbackInfo& info) {
                    auto [retProHolder, retProResolveRef, _] = maajs::MakePromise(info.Env());

                    maajs::CallMemberHelper<void>(
                        info.Env(),
                        newProPtr->Value(),
                        "then",
                        maajs::MakeFunction(
                            info.Env(),
                            "",
                            1,
                            [key, retProResolveRef](const maajs::CallbackInfo& info) {
                                auto self = info[0];
                                auto result = maajs::GetProp(info.Env(), maajs::ValueToObject(self), key.c_str());
                                maajs::CallFuncHelper<void>(info.Env(), retProResolveRef->Value(), result);
                                return maajs::MakeUndefined(info.Env());
                            },
                            [retProResolveRef](auto marker) { marker(retProResolveRef->Value()); }));

                    return retProHolder;
                },
                [newProPtr](auto marker) { marker(newProPtr->Value()); });
        }

        return newPro;
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

    void gc_mark([[maybe_unused]] maajs::NativeMarkerFunc marker) override { marker(source.Value()); }
};

MAA_JS_NATIVE_CLASS_STATIC_IMPL(JobImpl)

void JobImpl::init_proto(maajs::EnvType env, maajs::ConstObjectType proto)
{
    MAA_BIND_GETTER(env, proto, "source", JobImpl::get_source);
    MAA_BIND_GETTER(env, proto, "id", JobImpl::get_id);
    MAA_BIND_GETTER(env, proto, "status", JobImpl::get_status);
    MAA_BIND_FUNC(env, proto, "wait", JobImpl::wait);
}

maajs::ValueType load_job(maajs::EnvType env)
{
    maajs::ValueType ctor;
    maajs::NativeClass<JobImpl>::init(env, ctor);
    ExtContext::get(env)->jobCtor = maajs::PersistentFunction(env, ctor);
    return ctor;
}
