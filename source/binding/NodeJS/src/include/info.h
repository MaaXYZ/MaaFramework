#pragma once

#include <MaaFramework/MaaAPI.h>
#include <napi.h>

#include <future>
#include <map>

#if !defined(MAA_NODE_SERVER)
#include <MaaAgentClient/MaaAgentClientAPI.h>
#endif

struct CallbackContext
{
    Napi::Function fn;
    Napi::ThreadSafeFunction tsfn;
    const char* name;

    CallbackContext(Napi::Env env, Napi::Function cb, const char* name)
        : fn(cb)
        , tsfn(Napi::ThreadSafeFunction::New(env, fn, name, 0, 1))
        , name(name)
    {
    }

    ~CallbackContext() { tsfn.Release(); }

    template <typename Result>
    Result Call(std::function<Napi::Value(Napi::Env, Napi::Function)> caller, std::function<Result(Napi::Value)> parser)
    {
        std::promise<Result> promise;
        std::future<Result> future = promise.get_future();
        tsfn.BlockingCall([&caller, &parser, &promise](Napi::Env env, Napi::Function fn) {
            Napi::Value result = caller(env, fn);
            if (result.IsPromise()) {
                Napi::Object resultObject = result.As<Napi::Object>();
                resultObject.Get("then").As<Napi::Function>().Call(
                    resultObject,
                    {
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
                                info.Env().Undefined();
                            }),
                    });
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

template <typename Type, typename Impl>
struct InfoBase
{
    Type handle = nullptr;
};

#if !defined(MAA_NODE_SERVER)
struct AgentClientInfo : InfoBase<MaaAgentClient*, AgentClientInfo>
{
    constexpr static std::string_view name = "AgentClient";

    bool disposed = false;

    void dispose()
    {
        if (disposed) {
            return;
        }
        disposed = true;
        MaaAgentClientDestroy(handle);
    }

    ~AgentClientInfo() { dispose(); }
};
#endif

struct ControllerInfo : InfoBase<MaaController*, ControllerInfo>
{
    constexpr static std::string_view name = "Controller";

    CallbackContext* callback = nullptr;
    CallbackContext* custom_controller = nullptr;
    bool disposed = false;

    void dispose()
    {
        if (disposed) {
            return;
        }
        disposed = true;
        MaaControllerDestroy(handle);
        if (callback) {
            delete callback;
        }
        if (custom_controller) {
            delete custom_controller;
        }
    }

    ~ControllerInfo() { dispose(); }
};

struct ResourceInfo : InfoBase<MaaResource*, ResourceInfo>
{
    constexpr static std::string_view name = "Resource";

    CallbackContext* callback = nullptr;
    bool disposed = false;
    std::map<std::string, CallbackContext*> custom_recognizers = {};
    std::map<std::string, CallbackContext*> custom_actions = {};
#if !defined(MAA_NODE_SERVER)
    std::vector<Napi::Reference<Napi::External<AgentClientInfo>>> clients {};
#endif

    void dispose()
    {
        if (disposed) {
            return;
        }
        disposed = true;
        MaaResourceDestroy(handle);
        if (callback) {
            delete callback;
        }
        ClearRecos();
        ClearActs();
    }

    ~ResourceInfo() { dispose(); }

    void ClearRecos()
    {
        for (const auto& [_, cb] : custom_recognizers) {
            delete cb;
        }
        custom_recognizers.clear();
    }

    void ClearActs()
    {
        for (const auto& [_, cb] : custom_actions) {
            delete cb;
        }
        custom_actions.clear();
    }
};

struct TaskerInfo : InfoBase<MaaTasker*, TaskerInfo>
{
    constexpr static std::string_view name = "Tasker";

    CallbackContext* callback = nullptr;
    Napi::Reference<Napi::External<ResourceInfo>> resource = {};
    Napi::Reference<Napi::External<ControllerInfo>> controller = {};
    bool disposed = false;

    void dispose()
    {
        if (disposed) {
            return;
        }
        disposed = true;
        MaaTaskerDestroy(handle);
        if (callback) {
            delete callback;
        }
    }

    ~TaskerInfo() { dispose(); }
};

struct ExtContextInfo
{
    constexpr static std::string_view name = "ExtContextInfo";

    std::map<MaaTasker*, Napi::Reference<Napi::External<TaskerInfo>>> taskers;
    std::vector<std::unique_ptr<CallbackContext>> picli;
#if defined(MAA_NODE_SERVER)
    std::vector<std::unique_ptr<CallbackContext>> server;
#endif
};
