#pragma once

#include <functional>
#include <future>
#include <thread>

#include "../convert.h"
#include "../types.h"

namespace maajs
{

struct CallbackContext
{
    Napi::Function fn;
    Napi::ThreadSafeFunction tsfn;
    const char* name;
    Napi::FunctionReference owner_fn;
    const std::thread::id owner_thread;

    CallbackContext(Napi::Function cb, const char* name)
        : fn(cb)
        , tsfn(Napi::ThreadSafeFunction::New(cb.Env(), fn, name, 0, 1))
        , name(name)
        , owner_fn(Napi::Persistent(cb))
        , owner_thread(std::this_thread::get_id())
    {
    }

    ~CallbackContext() { tsfn.Release(); }

    template <typename Result>
    Result Call(std::function<Napi::Value(Napi::Function)> caller, std::function<Result(Napi::Value)> parser)
    {
        if (std::this_thread::get_id() == owner_thread) {
            // A synchronous getter cannot queue a callback to the JavaScript thread it is blocking.
            auto result = caller(owner_fn.Value());
            if (result.IsPromise()) {
                result.As<PromiseType>().Catch(
                    Napi::Function::New(result.Env(), [](const Napi::CallbackInfo& info) { return info.Env().Undefined(); }));
                throw MaaError { "A synchronous API cannot wait for an asynchronous custom callback" };
            }
            return parser(result);
        }

        std::promise<Result> promise;
        std::future<Result> future = promise.get_future();
        auto fail = [&promise]() {
            if constexpr (std::is_void_v<Result>) {
                promise.set_value();
            }
            else {
                promise.set_value(Result {});
            }
        };
        auto resolve = [&parser, &promise, fail](Napi::Value value) {
            try {
                if constexpr (std::is_void_v<Result>) {
                    parser(value);
                    promise.set_value();
                }
                else {
                    promise.set_value(parser(value));
                }
            }
            catch (const std::exception& error) {
                std::cerr << error.what() << std::endl;
                fail();
            }
        };
        auto status = tsfn.BlockingCall([&caller, resolve, fail](Napi::Env env, Napi::Function fn) {
            try {
                auto result = caller(fn);
                if (!result.IsPromise()) {
                    resolve(result);
                    return;
                }
                result.As<PromiseType>().Then(
                    Napi::Function::New(env, [resolve](const Napi::CallbackInfo& info) { resolve(info[0]); }),
                    Napi::Function::New(env, [fail](const Napi::CallbackInfo&) { fail(); }));
            }
            catch (const std::exception& error) {
                std::cerr << error.what() << std::endl;
                fail();
            }
        });
        if (status != napi_ok) {
            return Result();
        }
        return future.get();
    }

    template <typename Result>
    Result Call(std::function<ValueType(FunctionType)> caller)
    {
        if constexpr (std::is_same_v<Result, void>) {
            Call<void>(caller, [](maajs::ValueType) { });
        }
        else {
            return Call<Result>(caller, [](maajs::ValueType result) -> Result {
                try {
                    return maajs::JSConvert<Result>::from_value(result);
                }
                catch (const maajs::MaaError& err) {
                    std::cerr << err.what() << std::endl;
                    return { };
                }
            });
        }
    }
};

}
