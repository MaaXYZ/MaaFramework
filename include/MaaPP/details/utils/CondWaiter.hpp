#pragma once

#include <chrono>
#include <condition_variable>
#include <coroutine>
#include <iostream>
#include <mutex>
#include <thread>

#include "MaaPP/details/utils/EventLoop.hpp"

namespace maa::utils
{

class ConditionVariableAwaiter
{
public:
    ConditionVariableAwaiter(
        std::condition_variable& cv,
        std::mutex& mtx,
        std::function<bool()> pred)
        : cv_(cv)
        , mtx_(mtx)
        , pred_(pred)
    {
    }

    bool await_ready() noexcept
    {
        waited_ = pred_();
        return waited_;
    }

    void await_suspend(std::coroutine_handle<> handle)
    {
        std::thread([this, handle]() {
            std::unique_lock<std::mutex> lock(mtx_);
            waited_ = cv_.wait_for(lock, std::chrono::seconds(5), pred_);
            lock.unlock();

            std::cout << "cv_awaiter defer resume" << std::endl;
            EventLoop::current()->defer([handle]() {
                std::cout << "cv_awaiter call resume" << std::endl;
                handle.resume();
            });
        }).detach();
    }

    bool await_resume() noexcept { return waited_; }

private:
    std::condition_variable& cv_;
    std::mutex& mtx_;
    std::function<bool()> pred_;
    bool waited_ = false;
};

}
