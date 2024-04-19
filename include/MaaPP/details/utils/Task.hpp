#pragma once

#include "MaaPP/details/utils/FutureAwaiter.hpp"
#include <coroutine>
#include <future>

namespace maa::utils
{

template <typename T>
struct Task
{
    struct promise_type
    {
        T value;
        std::promise<T> promise;

        Task get_return_object()
        {
            return Task { std::coroutine_handle<promise_type>::from_promise(*this) };
        }

        std::suspend_never initial_suspend() { return {}; }

        std::suspend_never final_suspend() noexcept { return {}; }

        void return_value(T v)
        {
            value = v;
            promise.set_value(v);
        }

        void unhandled_exception() { promise.set_exception(std::current_exception()); }
    };

    std::coroutine_handle<promise_type> h;
    std::future<T> future_;

    Task(std::coroutine_handle<promise_type> h)
        : h(h)
        , future_(h.promise().promise.get_future())
    {
    }

    ~Task()
    {
        if (h) {
            h.destroy();
        }
    }

    auto operator co_await() { return FutureAwaiter<T>(future_); }
};

template <>
struct Task<void>
{
    struct promise_type
    {
        std::promise<void> promise;

        Task get_return_object()
        {
            return Task { std::coroutine_handle<promise_type>::from_promise(*this) };
        }

        std::suspend_never initial_suspend() { return {}; }

        std::suspend_never final_suspend() noexcept { return {}; }

        void return_void() { promise.set_value(); }

        void unhandled_exception() { promise.set_exception(std::current_exception()); }
    };

    std::coroutine_handle<promise_type> h;
    std::future<void> future_;

    Task(std::coroutine_handle<promise_type> h)
        : h(h)
        , future_(h.promise().promise.get_future())
    {
    }

    ~Task()
    {
        if (h) {
            h.destroy();
        }
    }

    auto operator co_await() { return FutureAwaiter<void>(future_); }
};

}
