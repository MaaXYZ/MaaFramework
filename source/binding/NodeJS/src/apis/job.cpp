#include "job.h"
#include "loader.h"

#include <MaaFramework/MaaAPI.h>

#include "../foundation/spec.h"
#include "ext.h"

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

    auto newPro =
        maajs::PromiseThen(pro, self.As<maajs::ObjectType>(), [](const maajs::CallbackInfo&, maajs::ObjectType self) { return self; });

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

                maajs::PromiseThen(
                    newProPtr->Value().As<maajs::PromiseType>(),
                    retProResolveRef->Value(),
                    [key](const maajs::CallbackInfo& info, maajs::FunctionType resolve) {
                        auto self = info[0];
                        auto result = self.As<maajs::ObjectType>()[key].AsValue();
                        maajs::CallFuncHelper<void>(resolve, result);
                        return info.Env().Undefined();
                    });

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

                    maajs::PromiseThen(
                        newProPtr->Value().As<maajs::PromiseType>(),
                        retProResolveRef->Value(),
                        [](const maajs::CallbackInfo& info, maajs::FunctionType resolve) {
                            auto self = info[0];
                            auto result = maajs::CallMemberHelper<maajs::ValueType>(self.As<maajs::ObjectType>(), "get");
                            maajs::CallFuncHelper<void>(resolve, result);
                            return info.Env().Undefined();
                        });

                    return retProHolder;
                },
                newProPtr));
    }

    return newPro;
}

maajs::ValueType JobImpl::get()
{
    return env.Undefined();
}

bool JobImpl::get_done()
{
    return get_succeeded() || get_failed();
}

bool JobImpl::get_succeeded()
{
    return get_status() == MaaStatus_Succeeded;
}

bool JobImpl::get_failed()
{
    return get_status() == MaaStatus_Failed;
}

bool JobImpl::get_running()
{
    return get_status() == MaaStatus_Running;
}

bool JobImpl::get_pending()
{
    return get_status() == MaaStatus_Pending;
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
    MAA_BIND_GETTER(proto, "done", JobImpl::get_done);
    MAA_BIND_GETTER(proto, "succeeded", JobImpl::get_succeeded);
    MAA_BIND_GETTER(proto, "failed", JobImpl::get_failed);
    MAA_BIND_GETTER(proto, "running", JobImpl::get_running);
    MAA_BIND_GETTER(proto, "pending", JobImpl::get_pending);
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
