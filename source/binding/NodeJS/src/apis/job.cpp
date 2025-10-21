#include "job.h"
#include "loader.h"

#include <MaaFramework/MaaAPI.h>

#include "../foundation/spec.h"
#include "ext.h"

MAA_JS_NATIVE_CLASS_STATIC_IMPL(JobImpl)

maajs::ValueType JobImpl::get_source()
{
    return source.Value();
}

MaaId JobImpl::get_id()
{
    return id;
}

MaaStatus JobImpl::get_status()
{
    if (!status) {
        status = maajs::CallMemberHelper<MaaStatus>(source.Value(), "status", id);
    }
    return *status;
}

maajs::PromiseType JobImpl::wait(maajs::ValueType self, maajs::EnvType)
{
    auto pro = maajs::CallMemberHelper<maajs::PromiseType>(source.Value(), "wait", id);

    auto selfPtr = std::make_shared<maajs::ObjectRefType>(maajs::PersistentObject(self.As<maajs::ObjectType>()));
    auto newPro = pro.Then(
        maajs::MakeFunction(
            env,
            "",
            1,
            [selfPtr](const maajs::CallbackInfo&) -> maajs::ValueType { return selfPtr->Value(); },
            [selfPtr](auto marker) { marker(selfPtr->Value()); }));

    std::vector<std::string> forwards = { "status", "done", "succeeded", "failed" };
    for (auto key : forwards) {
        auto newProPtr = std::make_shared<maajs::ObjectRefType>(maajs::PersistentObject(newPro));

        maajs::BindGetterSetter(
            newPro,
            key.c_str(),
            "job_forward",
            "",
            [key, newProPtr](const maajs::CallbackInfo& info) {
                auto [retProHolder, retProResolveRef, _] = maajs::MakePromise(info.Env());
                newProPtr->Value().As<maajs::PromiseType>().Then(
                    maajs::MakeFunction(
                        info.Env(),
                        "",
                        1,
                        [key, retProResolveRef](const maajs::CallbackInfo& info) {
                            auto self = info[0];
                            auto result = self.As<maajs::ObjectType>()[key].AsValue();
                            maajs::CallFuncHelper<void>(retProResolveRef->Value(), result);
                            return info.Env().Undefined();
                        },
                        [retProResolveRef](auto marker) { marker(retProResolveRef->Value()); }));

                return retProHolder;
            },
            nullptr,
            [newProPtr](auto marker) { marker(newProPtr->Value()); });
    }

    {
        auto newProPtr = std::make_shared<maajs::ObjectRefType>(maajs::PersistentObject(newPro));

        maajs::BindValue(
            newPro,
            "get",
            maajs::MakeFunction(
                env,
                "job_forward",
                0,
                [newProPtr](const maajs::CallbackInfo& info) {
                    auto [retProHolder, retProResolveRef, _] = maajs::MakePromise(info.Env());
                    newProPtr->Value().As<maajs::PromiseType>().Then(
                        maajs::MakeFunction(
                            info.Env(),
                            "",
                            1,
                            [retProResolveRef](const maajs::CallbackInfo& info) {
                                auto self = info[0];
                                auto result = maajs::CallMemberHelper<maajs::ValueType>(self.As<maajs::ObjectType>(), "get");
                                maajs::CallFuncHelper<void>(retProResolveRef->Value(), result);
                                return info.Env().Undefined();
                            },
                            [retProResolveRef](auto marker) { marker(retProResolveRef->Value()); }));

                    return retProHolder;
                },
                [newProPtr](auto marker) { marker(newProPtr->Value()); }));
    }

    return newPro;
}

maajs::ValueType JobImpl::get()
{
    return env.Undefined();
}

std::string JobImpl::to_string()
{
    return std::format(" source = {}, id = {} ", source.Value().ToString().Utf8Value(), id);
}

JobImpl* JobImpl::ctor(const maajs::CallbackInfo& info)
{
    auto job = new JobImpl;
    job->source = maajs::PersistentObject(info[0].As<maajs::ObjectType>());
    job->id = maajs::JSConvert<uint64_t>::from_value(info[1]);
    return job;
}

void JobImpl::init_proto(maajs::ObjectType proto, maajs::FunctionType)
{
    MAA_BIND_GETTER(proto, "source", JobImpl::get_source);
    MAA_BIND_GETTER(proto, "id", JobImpl::get_id);
    MAA_BIND_GETTER(proto, "status", JobImpl::get_status);
    MAA_BIND_FUNC(proto, "wait", JobImpl::wait);
}

void JobImpl::gc_mark(maajs::NativeMarkerFunc marker)
{
    marker(source.Value());
}

maajs::ValueType load_job(maajs::EnvType env)
{
    maajs::FunctionType ctor;
    maajs::NativeClass<JobImpl>::init(env, ctor);
    ExtContext::get(env)->jobCtor = maajs::PersistentFunction(ctor);
    return ctor;
}
