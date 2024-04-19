#pragma once

#include "MaaPP/details/utils/EventLoop.hpp"
#include <chrono>
#include <coroutine>
#include <future>

namespace maa::utils
{

template <typename T>
class FutureAwaiter
{
public:
    FutureAwaiter(std::future<T>& future)
        : future_(future)
    {
    }

    bool await_ready() noexcept
    {
        waited_ = future_.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
        return waited_;
    }

    void await_suspend(std::coroutine_handle<> handle)
    {
        std::thread([this, handle]() {
            waited_ = future_.wait_for(std::chrono::seconds(5)) == std::future_status::ready;

            EventLoop::current()->defer([handle]() { handle.resume(); });
        }).detach();
    }

    bool await_resume() noexcept { return waited_; }

private:
    std::future<T>& future_;
    bool waited_ = false;
};

}
