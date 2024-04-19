#pragma once

#include <condition_variable>
#include <coroutine>
#include <exception>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>

#include "MaaPP/details/utils/CondWaiter.hpp"
#include "MaaPP/details/utils/EventLoop.hpp"

namespace maa::utils
{

template <typename T>
struct Generator
{
    struct promise_type;
    using handle_type = std::coroutine_handle<promise_type>;

    struct promise_type
    {
        T value_;
        bool has_value_;
        std::mutex mtx_;
        std::condition_variable cond_;

        Generator get_return_object() { return Generator(handle_type::from_promise(*this)); }

        std::suspend_never initial_suspend() { return {}; }

        std::suspend_always final_suspend() noexcept { return {}; }

        void unhandled_exception() { std::rethrow_exception(std::current_exception()); }

        std::suspend_always yield_value(T value)
        {
            std::lock_guard<std::mutex> lock(mtx_);
            value_ = std::move(value);
            has_value_ = true;
            cond_.notify_one();
            return {};
        }

        void return_void() {}
    };

    handle_type handle_;

    Generator(handle_type handle)
        : handle_(handle)
    {
    }

    ~Generator()
    {
        if (handle_) {
            handle_.destroy();
        }
    }

    struct awaitable
    {
        Generator& generator;

        bool await_ready() { return false; }

        template <typename P>
        void await_suspend(std::coroutine_handle<P> caller)
        {
            std::thread([this, caller]() {
                std::unique_lock<std::mutex> lock(generator.handle_.promise().mtx_);
                generator.handle_.promise().cond_.wait(lock, [&] {
                    return generator.handle_.promise().has_value_;
                });
                lock.unlock();

                caller.resume();
            }).detach();
        }

        T await_resume()
        {
            std::lock_guard<std::mutex> lock(generator.handle_.promise().mtx_);
            if (!generator.handle_.promise().has_value_) {
                throw std::runtime_error("Generator::awaitable: No value");
            }
            T value = std::move(generator.handle_.promise().value_);
            generator.handle_.promise().has_value_ = false;
            EventLoop::current()->defer([this]() { generator.handle_.resume(); });
            return value;
        }
    };

    awaitable operator co_await() { return awaitable { *this }; }
};

template <typename T>
struct CachedGenerator
{
    std::queue<T> data_;
    std::mutex mtx_;
    std::condition_variable cond_;
    Generator<T> generator_;

    CachedGenerator()
        : generator_(generate())
    {
    }

    void push(T value)
    {
        std::lock_guard<std::mutex> lock(mtx_);
        data_.push(std::move(value));
        cond_.notify_one();
    }

    Generator<T> generate()
    {
        while (true) {
            bool waited = false;
            do {
                std::cout << "start wait cond" << std::endl;
                waited = co_await ConditionVariableAwaiter(cond_, mtx_, [this]() {
                    return !data_.empty();
                });
                std::cout << "wait cond " << waited << std::endl;
            } while (!waited);
            if (data_.empty()) {
                throw std::runtime_error("CachedGenerator: No value");
            }
            T value = std::move(data_.front());
            data_.pop();

            co_yield value;
        }
    }
};

}
