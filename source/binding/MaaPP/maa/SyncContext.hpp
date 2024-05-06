// IWYU pragma: private, include <MaaPP/MaaPP.hpp>

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

    maa::coro::Promise<bool> swipe(int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t duration)
    {
        return maa::coro::EventLoop::current()->eval([this, x1, y1, x2, y2, duration]() {
            return !!MaaSyncContextSwipe(handle_, x1, y1, x2, y2, duration);
        });
    }

    maa::coro::Promise<bool> press_key(int32_t key)
    {
        return maa::coro::EventLoop::current()->eval(
            [this, key]() { return !!MaaSyncContextPressKey(handle_, key); });
    }

    maa::coro::Promise<bool> press_key(std::string text)
    {
        return maa::coro::EventLoop::current()->eval(
            [this, text]() { return !!MaaSyncContextInputText(handle_, text.c_str()); });
    }

    maa::coro::Promise<bool> touch_down(int32_t contact, int32_t x, int32_t y, int32_t pressure)
    {
        return maa::coro::EventLoop::current()->eval([this, contact, x, y, pressure]() {
            return !!MaaSyncContextTouchDown(handle_, contact, x, y, pressure);
        });
    }

    maa::coro::Promise<bool> touch_move(int32_t contact, int32_t x, int32_t y, int32_t pressure)
    {
        return maa::coro::EventLoop::current()->eval([this, contact, x, y, pressure]() {
            return !!MaaSyncContextTouchMove(handle_, contact, x, y, pressure);
        });
    }

    maa::coro::Promise<bool> touch_up(int32_t contact)
    {
        return maa::coro::EventLoop::current()->eval(
            [this, contact]() { return !!MaaSyncContextTouchUp(handle_, contact); });
    }

    maa::coro::Promise<bool> screencap(std::shared_ptr<details::Image> image)
    {
        return maa::coro::EventLoop::current()->eval(
            [this, image]() { return !!MaaSyncContextScreencap(handle_, image->handle()); });
    }

    maa::coro::Promise<bool> cached_image(std::shared_ptr<details::Image> image)
    {
        return maa::coro::EventLoop::current()->eval(
            [this, image]() { return !!MaaSyncContextCachedImage(handle_, image->handle()); });
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
