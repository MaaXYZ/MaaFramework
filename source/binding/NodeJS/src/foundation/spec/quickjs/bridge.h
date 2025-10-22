#pragma once

#include <functional>
#include <map>
#include <memory>
#include <string>

#include "../types.h"

namespace maajs
{
struct NativeClassIDChain;
}

struct QuickJSRuntimeBridgeData
{
    JSClassID nativePointerHolderClassId;
    std::map<std::string, std::shared_ptr<maajs::NativeClassIDChain>> nativeClassId;

    static QuickJSRuntimeBridgeData* get(maajs::EnvType env);
    static QuickJSRuntimeBridgeData* get(JSRuntime* rt);
};

struct QuickJSRuntimeBridgeInterface
{
    enum SinkType
    {
        Resource,
        Controller,
        Tasker,
        Context,
    };

    using SinkFunc = void (*)(void* handle, const char* message, const char* details_json, void* callback_arg);

    QuickJSRuntimeBridgeData data;

    virtual ~QuickJSRuntimeBridgeInterface() = default;
    virtual void reg_task() = 0;
    virtual void push_task(std::function<void(JSContext* ctx)> task) = 0;
    virtual void call_exit(std::string result) = 0;

    virtual void plugin_add_sink(SinkType type, SinkFunc func, void* ptr) = 0;

    static QuickJSRuntimeBridgeInterface* get(maajs::EnvType env) { return get(env.Runtime()); }

    static QuickJSRuntimeBridgeInterface* get(JSRuntime* rt)
    {
        return static_cast<QuickJSRuntimeBridgeInterface*>(JS_GetRuntimeOpaque(rt));
    }
};

inline QuickJSRuntimeBridgeData* QuickJSRuntimeBridgeData::get(maajs::EnvType env)
{
    return get(env.Runtime());
}

inline QuickJSRuntimeBridgeData* QuickJSRuntimeBridgeData::get(JSRuntime* rt)
{
    return &QuickJSRuntimeBridgeInterface::get(rt)->data;
}
