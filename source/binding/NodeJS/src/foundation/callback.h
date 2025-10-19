#pragma once

#include <future>

#include "bridge.h"
#include "spec.h"

namespace maajs
{

#ifdef MAA_JS_IMPL_IS_NODEJS

struct CallbackContext
{
    Napi::Function fn;
    Napi::ThreadSafeFunction tsfn;
    const char* name;

    CallbackContext(Napi::Function cb, const char* name)
        : fn(cb)
        , tsfn(Napi::ThreadSafeFunction::New(cb.Env(), fn, name, 0, 1))
        , name(name)
    {
    }

    ~CallbackContext() { tsfn.Release(); }

    template <typename Result>
    Result Call(std::function<Napi::Value(Napi::Function)> caller, std::function<Result(Napi::Value)> parser)
    {
        std::promise<Result> promise;
        std::future<Result> future = promise.get_future();
        tsfn.BlockingCall([&caller, &parser, &promise](Napi::Env env, Napi::Function fn) {
            Napi::Value result = caller(fn);
            if (result.IsPromise()) {
                auto pro = result.As<PromiseType>();
                pro.Then(
                    Napi::Function::New(
                        env,
                        [&promise, &parser](const Napi::CallbackInfo& info) {
                            if constexpr (std::is_same_v<Result, void>) {
                                parser(info[0]);
                                promise.set_value();
                            }
                            else {
                                promise.set_value(parser(info[0]));
                            }
                            return info.Env().Undefined();
                        }),
                    // 即使reject依然尝试将其转化为目标值; 回调函数总是需要一个返回结果.
                    Napi::Function::New(env, [&promise, &parser](const Napi::CallbackInfo& info) {
                        if constexpr (std::is_same_v<Result, void>) {
                            parser(info[0]);
                            promise.set_value();
                        }
                        else {
                            promise.set_value(parser(info[0]));
                        }
                        info.Env().Undefined();
                    }));
            }
            else {
                if constexpr (std::is_same_v<Result, void>) {
                    parser(result);
                    promise.set_value();
                }
                else {
                    promise.set_value(parser(result));
                }
            }
        });
        return future.get();
    }
};

#endif

#ifdef MAA_JS_IMPL_IS_QUICKJS

struct CallbackContext
{
    FunctionType fn;
    const char* name;

    CallbackContext(FunctionType cb, const char* name)
        : fn(cb)
        , name(name)
    {
    }

    template <typename Result>
    Result Call(std::function<ValueType(FunctionType)> caller, std::function<Result(ValueType)> parser)
    {
        auto bridge = QuickJSRuntimeBridgeInterface::get(fn.Env());

        bridge->reg_task();

        std::promise<Result> promise;
        std::future<Result> future = promise.get_future();
        bridge->push_task([&caller, &parser, &promise, fn = fn](JSContext* ctx) {
            EnvType env { ctx };
            auto result = caller(fn);
            if (result.IsPromise()) {
                auto pro = result.As<PromiseType>();
                pro.Then(MakeFunction(env, "", 1, [&promise, &parser](const CallbackInfo& info) {
                    if constexpr (std::is_same_v<Result, void>) {
                        parser(info[0]);
                        promise.set_value();
                    }
                    else {
                        promise.set_value(parser(info[0]));
                    }
                    return info.Env().Undefined();
                }));
                pro.Catch(MakeFunction(env, "", 1, [&promise, &parser](const CallbackInfo& info) {
                    if constexpr (std::is_same_v<Result, void>) {
                        parser(info[0]);
                        promise.set_value();
                    }
                    else {
                        promise.set_value(parser(info[0]));
                    }
                    return info.Env().Undefined();
                }));
            }
            else {
                if constexpr (std::is_same_v<Result, void>) {
                    parser(result);
                    promise.set_value();
                }
                else {
                    promise.set_value(parser(result));
                }
            }
        });
        return future.get();
    }
};

#endif

}
