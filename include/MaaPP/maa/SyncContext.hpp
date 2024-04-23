#pragma once

#include <memory>
#include <string>

#include <MaaFramework/MaaAPI.h>
#include <meojson/json.hpp>

#include "MaaPP/coro/EventLoop.hpp"
#include "MaaPP/coro/Promise.hpp"

namespace maa
{

class SyncContext
{
    friend class CustomRecognizer;
    friend class CustomAction;

public:
    maa::coro::Promise<bool> run_task(const std::string& task_name, const json::object& param = {})
    {
        return maa::coro::EventLoop::current()->eval([this, &task_name, &param]() {
            return !!MaaSyncContextRunTask(handle_, task_name.c_str(), param.to_string().c_str());
        });
    }

    maa::coro::Promise<bool> click(int32_t x, int32_t y)
    {
        return maa::coro::EventLoop::current()->eval(
            [this, x, y]() { return !!MaaSyncContextClick(handle_, x, y); });
    }

private:
    static auto make(MaaSyncContextHandle handle)
    {
        auto ctx = std::make_shared<SyncContext>();
        ctx->handle_ = handle;
        return ctx;
    }

    MaaSyncContextHandle handle_;
};

}
