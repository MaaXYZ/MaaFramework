#pragma once

#include <memory>
#include <string>

#include <MaaFramework/MaaAPI.h>
#include <meojson/json.hpp>

#include "MaaPP/coro/EventLoop.hpp"
#include "MaaPP/coro/Promise.hpp"
#include "MaaPP/maa/Image.hpp"
#include "MaaPP/maa/Type.hpp"
#include "MaaPP/maa/details/String.hpp"

namespace maa
{

class SyncContext
{
    friend class CustomRecognizer;
    friend class CustomAction;

public:
    maa::coro::Promise<bool> run_task(std::string task_name, json::object param = {})
    {
        return maa::coro::EventLoop::current()->eval([this, task_name, param]() {
            return !!MaaSyncContextRunTask(handle_, task_name.c_str(), param.to_string().c_str());
        });
    }

    maa::coro::Promise<AnalyzeResult> run_recognition(
        std::shared_ptr<details::Image> img,
        std::string task_name,
        json::object param = {})
    {
        return maa::coro::EventLoop::current()->eval([this, img, task_name, param] {
            AnalyzeResult result;
            details::String buffer;
            result.result = MaaSyncContextRunRecognition(
                handle_,
                img->handle(),
                task_name.c_str(),
                param.to_string().c_str(),
                &result.rec_box,
                buffer.handle());
            result.rec_detail = buffer;
            return result;
        });
    }

    maa::coro::Promise<bool> run_action(
        MaaRect rec_box,
        std::string rec_details,
        std::string task_name,
        json::object param = {})
    {
        return maa::coro::EventLoop::current()->eval(
            [this, rec_box, rec_details, task_name, param] {
                return !!MaaSyncContextRunAction(
                    handle_,
                    task_name.c_str(),
                    param.to_string().c_str(),
                    const_cast<MaaRectHandle>(&rec_box),
                    rec_details.c_str());
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
