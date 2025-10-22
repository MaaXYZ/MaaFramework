#include "loader.h"

#include <MaaAgentServer/MaaAgentServerAPI.h>

#include "../foundation/spec.h"
#include "callback.h"
#include "ext.h"

static void register_custom_recognition(maajs::EnvType env, std::string key, maajs::FunctionType func)
{
    auto ctx = std::make_unique<maajs::CallbackContext>(func, "CustomReco");
    if (!MaaAgentServerRegisterCustomRecognition(key.c_str(), CustomReco, ctx.get())) {
        throw maajs::MaaError { "Server register_custom_recognition failed" };
    }
    ExtContext::get(env)->globalCallbacks.push_back(std::move(ctx));
}

static void register_custom_action(maajs::EnvType env, std::string key, maajs::FunctionType func)
{
    auto ctx = std::make_unique<maajs::CallbackContext>(func, "CustomAct");
    if (!MaaAgentServerRegisterCustomAction(key.c_str(), CustomAct, ctx.get())) {
        throw maajs::MaaError { "Server register_custom_action failed" };
    }
    ExtContext::get(env)->globalCallbacks.push_back(std::move(ctx));
}

static void add_resource_sink(maajs::FunctionType func)
{
    auto ctx = std::make_unique<maajs::CallbackContext>(func, "ResourceSink");
    MaaAgentServerAddResourceSink(ResourceSink, ctx.get());
    ExtContext::get(func.Env())->globalCallbacks.push_back(std::move(ctx));
}

static void add_controller_sink(maajs::FunctionType func)
{
    auto ctx = std::make_unique<maajs::CallbackContext>(func, "ControllerSink");
    MaaAgentServerAddControllerSink(ControllerSink, ctx.get());
    ExtContext::get(func.Env())->globalCallbacks.push_back(std::move(ctx));
}

static void add_tasker_sink(maajs::FunctionType func)
{
    auto ctx = std::make_unique<maajs::CallbackContext>(func, "TaskerSink");
    MaaAgentServerAddTaskerSink(TaskerSink, ctx.get());
    ExtContext::get(func.Env())->globalCallbacks.push_back(std::move(ctx));
}

static void add_context_sink(maajs::FunctionType func)
{
    auto ctx = std::make_unique<maajs::CallbackContext>(func, "ContextSink");
    MaaAgentServerAddContextSink(ContextSink, ctx.get());
    ExtContext::get(func.Env())->globalCallbacks.push_back(std::move(ctx));
}

static maajs::PromiseType start_up(maajs::EnvType env, std::string identifier)
{
    auto work = new maajs::AsyncWork<bool>(env, [identifier]() { return MaaAgentServerStartUp(identifier.c_str()); });
    work->Queue();
    return work->Promise();
}

static maajs::PromiseType shut_down(maajs::EnvType env)
{
    auto work = new maajs::AsyncWork<std::monostate>(env, []() {
        MaaAgentServerShutDown();
        return std::monostate {};
    });
    work->Queue();
    return work->Promise();
}

static maajs::PromiseType join(maajs::EnvType env)
{
    auto work = new maajs::AsyncWork<std::monostate>(env, []() {
        MaaAgentServerJoin();
        return std::monostate {};
    });
    work->Queue();
    return work->Promise();
}

static void detach()
{
    MaaAgentServerDetach();
}

maajs::ValueType load_server(maajs::EnvType env)
{
    auto obj = maajs::ObjectType::New(env);

    MAA_BIND_FUNC(obj, "register_custom_recognition", register_custom_recognition);
    MAA_BIND_FUNC(obj, "register_custom_action", register_custom_action);
    MAA_BIND_FUNC(obj, "add_resource_sink", add_resource_sink);
    MAA_BIND_FUNC(obj, "add_controller_sink", add_controller_sink);
    MAA_BIND_FUNC(obj, "add_tasker_sink", add_tasker_sink);
    MAA_BIND_FUNC(obj, "add_context_sink", add_context_sink);
    MAA_BIND_FUNC(obj, "start_up", start_up);
    MAA_BIND_FUNC(obj, "shut_down", shut_down);
    MAA_BIND_FUNC(obj, "join", join);
    MAA_BIND_FUNC(obj, "detach", detach);

    return obj;
}
