#pragma once

#include <functional>
#include <string>

#include "../types.h"

struct QuickJSRuntimeBridgeInterface
{
    virtual ~QuickJSRuntimeBridgeInterface() = default;
    virtual void reg_task() = 0;
    virtual void push_task(std::function<void(JSContext* ctx)> task) = 0;
    virtual void call_exit(std::string result) = 0;

    static QuickJSRuntimeBridgeInterface* get(maajs::EnvType env)
    {
        return static_cast<QuickJSRuntimeBridgeInterface*>(JS_GetRuntimeOpaque(JS_GetRuntime(env)));
    }
};
