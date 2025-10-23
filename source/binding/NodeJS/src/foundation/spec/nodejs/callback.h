#pragma once

#include <functional>
#include <future>

#include "../convert.h"
#include "../types.h"

namespace maajs
{

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

    template <typename Result>
    Result Call(std::function<ValueType(FunctionType)> caller)
    {
        if constexpr (std::is_same_v<Result, void>) {
            Call<void>(caller, [](maajs::ValueType) {});
        }
        else {
            return Call<Result>(caller, [](maajs::ValueType result) -> Result {
                try {
                    return maajs::JSConvert<Result>::from_value(result);
                }
                catch (const maajs::MaaError& err) {
                    std::cerr << err.what() << std::endl;
                    return {};
                }
            });
        }
    }
};

}
