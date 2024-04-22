#pragma once

#include <functional>

#include "MaaPP/coro/Promise.hpp"
#include "MaaPP/coro/ThreadPool.hpp"

namespace maa::coro
{

class EventLoop
{
public:
    EventLoop(size_t count = 8)
        : pool(count)
    {
        current_ = this;
    }

    ~EventLoop() { current_ = nullptr; }

    EventLoop(const EventLoop&) = delete;
    EventLoop& operator=(const EventLoop&) = delete;

    template <typename F>
    void defer(F f)
    {
        pool.defer(f);
    }

    void defer_stop()
    {
        defer([this]() { this->stop(); });
    }

    template <typename F>
    auto eval(F f) -> maa::coro::Promise<std::invoke_result_t<F>>
    {
        using R = std::invoke_result_t<F>;
        std::function<R()> func(f);
        auto result_pro = maa::coro::Promise<R>();

        defer([result_pro, func]() {
            auto result = func();
            maa::coro::EventLoop::current()->defer(
                [result_pro, result = std::move(result)]() { result_pro.resolve(result); });
        });

        return result_pro;
    }

    void exec() { pool.exec(); }

    void stop() { pool.stop(); }

    static EventLoop* current() { return current_; }

private:
    static inline EventLoop* current_ = nullptr;

    ThreadPool pool;
};

}
