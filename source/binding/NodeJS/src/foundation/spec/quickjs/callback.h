#pragma once

#include <functional>
#include <future>

#include "../convert.h"
#include "../types.h"
#include "bridge.h"

namespace maajs
{

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
