// IWYU pragma: private, include <MaaPP/MaaPP.hpp>

#pragma once

#include "MaaPP/module/Module.h"

#ifndef MAAPP_USE_MODULE

#include <coroutine>
#include <functional>
#include <future>
#include <mutex>
#include <optional>
#include <utility>
#include <vector>

#endif

namespace maa::coro
{

MAAPP_EXPORT template <typename T>
struct Promise;

MAAPP_EXPORT template <typename T>
inline Promise<T> resolve_now(T&& value);

MAAPP_EXPORT inline Promise<void> resolve_now();

namespace details
{

template <typename T>
struct promise_traits
{
    using then_t = void(const T&);
    using result_t = T;
    template <typename F>
    using then_ret_t = std::invoke_result_t<F, const T&>;
    template <typename R>
    using then_holder_t = R(const T&);
};

template <>
struct promise_traits<void>
{
    using then_t = void();
    using result_t = std::monostate;
    template <typename F>
    using then_ret_t = std::invoke_result_t<F>;
    template <typename R>
    using then_holder_t = R();
};

template <typename T>
struct promise_type;

}

MAAPP_EXPORT template <typename T = void>
struct Promise
{
    using value_t = T;
    using then_t = typename details::promise_traits<T>::then_t;
    using result_t = typename details::promise_traits<T>::result_t;
    template <typename F>
    using then_ret_t = typename details::promise_traits<T>::template then_ret_t<F>;
    template <typename R>
    using then_holder_t = typename details::promise_traits<T>::template then_holder_t<R>;

    struct State
    {
        std::optional<result_t> result_;
        std::vector<std::function<then_t>> then_;
        std::mutex mtx_;

        std::optional<std::coroutine_handle<>> task_;

        State() {}

        State(const State&) = delete;
        State& operator=(const State&) = delete;

        ~State()
        {
            if (task_.has_value()) {
                task_.value().destroy();
            }
        }
    };

    std::shared_ptr<State> state_;

    Promise()
        : state_(std::make_shared<State>())
    {
    }

    bool resolved_noguard() const { return state_->result_.has_value(); }

    bool resolved() const
    {
        std::lock_guard<std::mutex> lock(state_->mtx_);
        return resolved_noguard();
    }

    template <typename F>
    auto then(F f) const -> Promise<then_ret_t<F>>
    {
        using R = then_ret_t<F>;
        std::function<then_holder_t<R>> func(f);
        std::unique_lock<std::mutex> lock(state_->mtx_);
        if (resolved_noguard()) {
            lock.unlock();
            if constexpr (std::is_same_v<void, R>) {
                if constexpr (std::is_same_v<void, T>) {
                    func();
                }
                else {
                    func(state_->result_.value());
                }
                return resolve_now();
            }
            else {
                if constexpr (std::is_same_v<void, T>) {
                    return resolve_now<R>(func());
                }
                else {
                    return resolve_now<R>(func(state_->result_.value()));
                }
            }
        }
        else {
            Promise<R> pro;
            if constexpr (std::is_same_v<void, T>) {
                state_->then_.push_back([p = pro, f = std::move(func)]() mutable {
                    if constexpr (std::is_same_v<void, R>) {
                        f();
                        p.resolve();
                    }
                    else {
                        p.resolve(f());
                    }
                });
            }
            else {
                state_->then_.push_back([p = pro, f = std::move(func)](const T& val) mutable {
                    if constexpr (std::is_same_v<void, R>) {
                        f(val);
                        p.resolve();
                    }
                    else {
                        p.resolve(f(val));
                    }
                });
            }
            return pro;
        }
    }

    template <typename t = T>
    requires std::is_same_v<void, t>
    void resolve() const
    {
        if (resolved()) {
            return;
        }
        {
            std::lock_guard<std::mutex> lock(state_->mtx_);
            state_->result_ = std::monostate {};
        }
        std::vector<std::function<then_t>> thens;
        thens.swap(state_->then_);
        for (const auto& f : thens) {
            f();
        }
    }

    template <typename t = T>
    requires(!std::is_same_v<void, t>)
    void resolve(t value) const
    {
        if (resolved()) {
            return;
        }
        {
            std::lock_guard<std::mutex> lock(state_->mtx_);
            state_->result_ = std::move(value);
        }
        std::vector<std::function<then_t>> thens;
        thens.swap(state_->then_);
        for (const auto& f : thens) {
            f(state_->result_.value());
        }
    }

    T sync_wait()
    {
        std::promise<T> result;
        if constexpr (std::is_same_v<void, T>) {
            then([&result]() { result.set_value(); });
        }
        else {
            then([&result](T t) { result.set_value(std::move(t)); });
        }
        auto future = result.get_future();
        future.wait();
        if constexpr (!std::is_same_v<void, T>) {
            return future.get();
        }
    }

    using promise_type = details::promise_type<T>;

    bool await_ready() const { return resolved(); }

    void await_suspend(std::coroutine_handle<> handle) const
    {
        if constexpr (std::is_same_v<void, T>) {
            then([handle]() { handle.resume(); });
        }
        else {
            then([handle](const T&) { handle.resume(); });
        }
    }

    T await_resume() const
    {
        if constexpr (std::is_same_v<void, T>) {
            ;
        }
        else {
            return state_->result_.value();
        }
    }
};

namespace details
{

struct __promise_type_base
{
    std::suspend_never initial_suspend() noexcept { return {}; }

    std::suspend_always final_suspend() noexcept { return {}; }

    void unhandled_exception() { std::rethrow_exception(std::current_exception()); }
};

template <typename T>
struct promise_type : public __promise_type_base
{
    Promise<T> promise_;

    Promise<T> get_return_object()
    {
        promise_.state_->task_ = std::coroutine_handle<promise_type>::from_promise(*this);
        return promise_;
    }

    void return_value(T val) { promise_.resolve(std::move(val)); }
};

template <>
struct promise_type<void> : public __promise_type_base
{
    Promise<void> promise_;

    Promise<void> get_return_object()
    {
        promise_.state_->task_ = std::coroutine_handle<promise_type>::from_promise(*this);
        return promise_;
    }

    void return_void() { promise_.resolve(); }
};

}

MAAPP_EXPORT template <typename T>
inline Promise<T> resolve_now(T&& value)
{
    Promise<T> pro;
    pro.resolve(std::forward<T>(value));
    return pro;
}

MAAPP_EXPORT inline Promise<> resolve_now()
{
    Promise<> pro;
    pro.resolve();
    return pro;
}

}
