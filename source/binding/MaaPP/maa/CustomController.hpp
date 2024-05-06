// IWYU pragma: private, include <MaaPP/MaaPP.hpp>

#pragma once

#include <memory>

#include <MaaFramework/MaaAPI.h>

#include "MaaPP/coro/Promise.hpp"
#include "MaaPP/maa/Image.hpp"

namespace maa
{

struct CustomControllerAPI
{
    virtual ~CustomControllerAPI() = default;

    virtual coro::Promise<bool> connect() { co_return false; }

    virtual coro::Promise<std::optional<std::string>> request_uuid() { co_return std::nullopt; }

    virtual coro::Promise<std::optional<std::tuple<int32_t, int32_t>>> request_resolution()
    {
        co_return std::nullopt;
    }

    virtual coro::Promise<bool> start_app([[maybe_unused]] std::string intent) { co_return false; }

    virtual coro::Promise<bool> stop_app([[maybe_unused]] std::string intent) { co_return false; }

    virtual coro::Promise<bool> screencap([[maybe_unused]] std::shared_ptr<details::Image> image)
    {
        co_return false;
    }

    virtual coro::Promise<bool> click([[maybe_unused]] int32_t x, [[maybe_unused]] int32_t y)
    {
        co_return false;
    }

    virtual coro::Promise<bool> swipe(
        [[maybe_unused]] int32_t x1,
        [[maybe_unused]] int32_t y1,
        [[maybe_unused]] int32_t x2,
        [[maybe_unused]] int32_t y2,
        [[maybe_unused]] int32_t duration)
    {
        co_return false;
    }

    virtual coro::Promise<bool> touch_down(
        [[maybe_unused]] int32_t contact,
        [[maybe_unused]] int32_t x,
        [[maybe_unused]] int32_t y,
        [[maybe_unused]] int32_t pressure)
    {
        co_return false;
    }

    virtual coro::Promise<bool> touch_move(
        [[maybe_unused]] int32_t contact,
        [[maybe_unused]] int32_t x,
        [[maybe_unused]] int32_t y,
        [[maybe_unused]] int32_t pressure)
    {
        co_return false;
    }

    virtual coro::Promise<bool> touch_up([[maybe_unused]] int32_t contact) { co_return false; }

    virtual coro::Promise<bool> press_key([[maybe_unused]] int32_t keycode) { co_return false; }

    virtual coro::Promise<bool> input_text([[maybe_unused]] std::string text) { co_return false; }
};

}
