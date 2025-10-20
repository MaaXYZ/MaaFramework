#pragma once

#include <functional>
#include <memory>
#include <thread>

#include "../../bridge.h"
#include "../../convert.h"

namespace maajs
{

template <typename Ret>
struct AsyncWork
{
    AsyncWork(EnvType env, std::function<Ret()> exec)
        : env(env)
        , exec(exec)
    {
        auto [pro, res, rej] = MakePromise(env);
        promise = pro;
        resolve = res;
        reject = rej;
    }

    void Queue()
    {
        auto bridge = QuickJSRuntimeBridgeInterface::get(env);

        bridge->reg_task();

        (std::thread {
             [bridge, exec = exec, resolve = resolve, reject = reject, worker = this]() {
                 Ret result = exec();
                 bridge->push_task([result, resolve, reject, worker = std::shared_ptr<AsyncWork<Ret>>(worker)](JSContext* ctx) {
                     QjsEnv env { ctx };
                     try {
                         auto val = JSConvert<Ret>::to_value(env, result);
                         maajs::CallFuncHelper<void>(resolve->Value(), val);
                     }
                     catch (const MaaError& exc) {
                         maajs::CallFuncHelper<void>(reject->Value(), std::string { exc.what() });
                     }
                 });
             },
         })
            .detach();
    }

    PromiseType Promise() { return promise; }

    EnvType env;
    std::function<Ret()> exec;
    PromiseType promise;
    std::shared_ptr<FunctionRefType> resolve;
    std::shared_ptr<FunctionRefType> reject;
};

}
