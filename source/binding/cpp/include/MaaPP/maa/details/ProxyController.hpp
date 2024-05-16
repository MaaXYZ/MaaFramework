// IWYU pragma: private, include <MaaPP/MaaPP.hpp>

#pragma once

#include "MaaFramework/MaaDef.h"
#include "MaaPP/coro/Promise.hpp"
#include "MaaPP/maa/Controller.hpp"

namespace maa::details
{

struct ProxyController : public CustomControllerAPI
{
    std::shared_ptr<Controller> target_;
    int32_t width_ = 1080, height_ = 720;

    ProxyController(std::shared_ptr<Controller> target)
        : target_(target)
    {
    }

    virtual coro::Promise<bool> connect() override
    {
        co_return co_await target_->post_connect()->wait() == MaaStatus_Success;
    }

    virtual coro::Promise<std::optional<std::string>> request_uuid() override
    {
        co_return target_->uuid();
    }

    virtual coro::Promise<std::optional<std::tuple<int32_t, int32_t>>> request_resolution() override
    {
        co_return std::make_tuple(width_, height_);
    }

    virtual coro::Promise<bool> start_app([[maybe_unused]] std::string intent) override
    {
        co_return co_await target_->post_start_app(intent)->wait() == MaaStatus_Success;
    }

    virtual coro::Promise<bool> stop_app(std::string intent) override
    {
        co_return co_await target_->post_stop_app(intent)->wait() == MaaStatus_Success;
    }

    virtual coro::Promise<bool> screencap(std::shared_ptr<details::Image> image) override
    {
        co_await target_->post_screencap()->wait();
        target_->image(image);
        co_return true;
    }

    virtual coro::Promise<bool> click(int32_t x, int32_t y) override
    {
        co_return co_await target_->post_click(x, y)->wait() == MaaStatus_Success;
    }

    virtual coro::Promise<bool>
        swipe(int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t duration) override
    {
        co_return co_await target_->post_swipe(x1, y1, x2, y2, duration)->wait()
            == MaaStatus_Success;
    }

    virtual coro::Promise<bool>
        touch_down(int32_t contact, int32_t x, int32_t y, int32_t pressure) override
    {
        co_return co_await target_->post_touch_down(contact, x, y, pressure)->wait()
            == MaaStatus_Success;
    }

    virtual coro::Promise<bool>
        touch_move(int32_t contact, int32_t x, int32_t y, int32_t pressure) override
    {
        co_return co_await target_->post_touch_move(contact, x, y, pressure)->wait()
            == MaaStatus_Success;
    }

    virtual coro::Promise<bool> touch_up(int32_t contact) override
    {
        co_return co_await target_->post_touch_up(contact)->wait() == MaaStatus_Success;
    }

    virtual coro::Promise<bool> press_key(int32_t keycode) override
    {
        co_return co_await target_->post_press_key(keycode)->wait() == MaaStatus_Success;
    }

    virtual coro::Promise<bool> input_text(std::string text) override
    {
        co_return co_await target_->post_input_text(text)->wait() == MaaStatus_Success;
    }
};

}
