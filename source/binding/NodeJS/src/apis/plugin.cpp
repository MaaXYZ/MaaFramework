#include "loader.h"

#include <MaaAgentServer/MaaAgentServerAPI.h>

#include "../foundation/spec.h"
#include "callback.h"
#include "ext.h"

static void add_resource_sink(maajs::FunctionType func)
{
    auto ctx = std::make_unique<maajs::CallbackContext>(func, "ResourceSink");
    QuickJSRuntimeBridgeInterface::get(func.Env())->plugin_add_sink(QuickJSRuntimeBridgeInterface::Resource, ResourceSink, ctx.get());
    ExtContext::get(func.Env())->globalCallbacks.push_back(std::move(ctx));
}

static void add_controller_sink(maajs::FunctionType func)
{
    auto ctx = std::make_unique<maajs::CallbackContext>(func, "ControllerSink");
    QuickJSRuntimeBridgeInterface::get(func.Env())->plugin_add_sink(QuickJSRuntimeBridgeInterface::Controller, ControllerSink, ctx.get());
    ExtContext::get(func.Env())->globalCallbacks.push_back(std::move(ctx));
}

static void add_tasker_sink(maajs::FunctionType func)
{
    auto ctx = std::make_unique<maajs::CallbackContext>(func, "TaskerSink");
    QuickJSRuntimeBridgeInterface::get(func.Env())->plugin_add_sink(QuickJSRuntimeBridgeInterface::Tasker, TaskerSink, ctx.get());
    ExtContext::get(func.Env())->globalCallbacks.push_back(std::move(ctx));
}

static void add_context_sink(maajs::FunctionType func)
{
    auto ctx = std::make_unique<maajs::CallbackContext>(func, "ContextSink");
    QuickJSRuntimeBridgeInterface::get(func.Env())->plugin_add_sink(QuickJSRuntimeBridgeInterface::Context, ContextSink, ctx.get());
    ExtContext::get(func.Env())->globalCallbacks.push_back(std::move(ctx));
}

maajs::ValueType load_plugin(maajs::EnvType env)
{
    auto obj = maajs::ObjectType::New(env);

    MAA_BIND_FUNC(obj, "add_resource_sink", add_resource_sink);
    MAA_BIND_FUNC(obj, "add_controller_sink", add_controller_sink);
    MAA_BIND_FUNC(obj, "add_tasker_sink", add_tasker_sink);
    MAA_BIND_FUNC(obj, "add_context_sink", add_context_sink);

    return obj;
}
