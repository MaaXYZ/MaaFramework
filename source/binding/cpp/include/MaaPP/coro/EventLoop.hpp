// IWYU pragma: private, include <MaaPP/MaaPP.hpp>

#pragma once

#include <chrono>

#include "MaaPP/coro/Promise.hpp"
#include "MaaPP/coro/ThreadPool.hpp"

namespace maa::coro
{

class EventLoop
{
public:
    EventLoop(size_t count = 8)
        : pool_(count)
    {
        current_ = this;
    }

    ~EventLoop() { current_ = nullptr; }

    EventLoop(const EventLoop&) = delete;
    EventLoop& operator=(const EventLoop&) = delete;

    template <typename F>
    void defer(F f)
    {
        pool_.defer(f);
    }

    Promise<void> sleep(std::chrono::seconds times)
    {
        Promise<void> pro;
        defer([pro, times]() {
            std::this_thread::sleep_for(times);
            pro.resolve();
        });
        return pro;
    }

    template <typename F>
    auto eval(F f) -> Promise<std::invoke_result_t<F>>
    {
        using R = std::invoke_result_t<F>;
        std::function<R()> func(std::move(f));
        auto result_pro = Promise<R>();

        defer([result_pro, func = std::move(func)]() {
            auto result = func();
            EventLoop::current()->defer(
                [result_pro, result = std::move(result)]() { result_pro.resolve(result); });
        });

        return result_pro;
    }

    int exec()
    {
        pool_.exec();
        return code_;
    }

    void stop(int code = 0)
    {
        pool_.stop();
        code_ = code;
    }

    void stop_after(Promise<int> pro)
    {
        pro.then([this](int code) { stop(code); });
    }

    void stop_after(Promise<> pro)
    {
        pro.then([this]() { stop(0); });
    }

    static EventLoop* current() { return current_; }

private:
    static inline EventLoop* current_ = nullptr;

    ThreadPool pool_;
    int code_ = 0;
};

}
