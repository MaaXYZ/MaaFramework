// IWYU pragma: private, include <MaaPP/MaaPP.hpp>

#pragma once

#include <atomic>
#include <memory>
#include <variant>
#include <vector>

#include "MaaPP/coro/Promise.hpp"
#include "MaaPP/coro/details/Utils.hpp"

namespace maa::coro
{

namespace details
{

template <typename Pro>
struct DynamicMergeState
{
    using promise_type = Pro;
    using value_type = typename promise_value<Pro>::type;

    std::vector<Pro> promises = {};
    std::vector<value_type> result = {};
    std::atomic<size_t> counter = 0;
};

template <typename Fulfill, typename State>
inline void dynamic_merge_helper(
    std::shared_ptr<Fulfill> fulfill,
    std::shared_ptr<State> state,
    size_t index)
{
    using element_type = typename State::value_type;
    if constexpr (std::is_same_v<void, element_type>) {
        state->promises[index].then([=]() {
            state->result[index] = std::monostate {};
            fulfill->hit(state, index);
        });
    }
    else {
        state->promises[index].then([=](element_type value) {
            state->result[index] = std::move(value);
            fulfill->hit(state, index);
        });
    }
}

template <typename Fulfill, typename Pro>
inline void dynamic_merge(std::shared_ptr<Fulfill> fulfill, std::vector<Pro> pros)
{
    using State = DynamicMergeState<Pro>;
    auto state = std::make_shared<State>();
    state->promises = std::move(pros);

    for (size_t i = 0; i < state->promises.size(); i++) {
        dynamic_merge_helper(fulfill, state, i);
    }
}

template <typename State>
struct dynamic_merge_all_fulfill
{
    Promise<std::vector<typename State::value_type>> promise;

    void hit(std::shared_ptr<State> state, [[maybe_unused]] size_t index)
    {
        if (++state->counter == state->promises.size()) {
            promise.resolve(std::move(state->result));
        }
    }
};

template <typename State>
struct dynamic_merge_any_fulfill
{
    using result_t = std::tuple<size_t, typename State::value_type>;
    Promise<result_t> promise;

    void hit(std::shared_ptr<State> state, size_t index)
    {
        if (++state->counter == 1) {
            result_t result = std::make_tuple(index, std::move(state->result[index]));
            promise.resolve(std::move(result));
        }
    }
};

}

template <typename Pro>
inline auto all(std::vector<Pro> pros)
{
    auto fulfill =
        std::make_shared<details::dynamic_merge_all_fulfill<details::DynamicMergeState<Pro>>>();
    details::dynamic_merge(fulfill, pros);
    return fulfill->promise;
}

template <typename Pro>
inline auto any(Pro pros)
{
    auto fulfill =
        std::make_shared<details::dynamic_merge_all_fulfill<details::DynamicMergeState<Pro>>>();
    details::dynamic_merge(fulfill, pros);
    return fulfill->promise;
}

}
