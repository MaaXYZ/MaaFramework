#pragma once

#include <thread>

#include "bridge.h"
#include "convert.h"
#include "spec.h"

namespace maajs
{

#ifdef MAA_JS_IMPL_IS_NODEJS

template <typename Ret>
struct AsyncWork : public Napi::AsyncWorker
{
    AsyncWork(EnvType env, std::function<Ret()> exec)
        : Napi::AsyncWorker(env)
        , exec(exec)
        , deferred(env)
    {
    }

    void Execute() override { result = exec(); }

    void OnOK() override
    {
        try {
            deferred.Resolve(JSConvert<Ret>::to_value(Env(), result));
        }
        catch (const ConvertFailed& exc) {
            deferred.Reject(Napi::String::New(Env(), exc.what()));
        }
    }

    void OnError(const Napi::Error& err) override { deferred.Reject(err.Value()); }

    PromiseType Promise() { return deferred.Promise(); }

    std::function<Ret()> exec;
    Ret result;
    Napi::Promise::Deferred deferred;
};

#endif

#ifdef MAA_JS_IMPL_IS_QUICKJS

template <typename Ret>
struct AsyncWork
{
    AsyncWork(EnvType env, std::function<Ret()> exec)
        : env(env)
        , exec(exec)
    {
        auto [pro, res, rej] = maajs::MakePromise(env);
        promise = pro;
        resolve = std::move(res);
        reject = std::move(rej);
    }

    void Queue()
    {
        auto bridge = QuickJSRuntimeBridgeInterface::get(env);

        (std::thread {
             [bridge, exec = exec, resolve = resolve, reject = reject, worker = this]() {
                 Ret result = exec();
                 bridge->push_task([result, resolve, reject, worker](JSContext* ctx) {
                     maajs::EnvType env {
                         JS_GetRuntime(ctx),
                         ctx,
                     };

                     try {
                         auto val = JSConvert<Ret>::to_value(env, result);
                         maajs::CallFuncHelper<void>(env, resolve->Value(), val);
                     }
                     catch (const ConvertFailed& exc) {
                         maajs::CallFuncHelper<void>(env, reject->Value(), MakeString(env, exc.what()));
                     }

                     delete worker;
                 });
             },
         })
            .detach();
    }

    PromiseType Promise() { return promise; }

    EnvType env;
    std::function<Ret()> exec;
    PromiseType promise;
    std::shared_ptr<QjsRef> resolve;
    std::shared_ptr<QjsRef> reject;
};

#endif

}
