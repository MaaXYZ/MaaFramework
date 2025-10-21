#pragma once

#include <functional>

#include "../utils.h"

namespace maajs
{
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
        catch (const MaaError& exc) {
            deferred.Reject(Napi::String::New(Env(), exc.what()));
        }
    }

    void OnError(const Napi::Error& err) override { deferred.Reject(err.Value()); }

    PromiseType Promise() { return deferred.Promise(); }

    std::function<Ret()> exec;
    Ret result;
    Napi::Promise::Deferred deferred;
};
}
