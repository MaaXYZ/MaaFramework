// IWYU pragma: private, include "MaaPP/MaaPP.hpp"

#pragma once

#include <atomic>
#include <memory>
#include <variant>

#include "MaaPP/coro/Promise.hpp"
#include "MaaPP/coro/details/Utils.hpp"

namespace maa::coro
{

namespace details
{

template <typename ProTuple>
using pro_value_tuple =
    transform_tuple<transform_tuple<ProTuple, promise_value, std::tuple>, wrap_void, std::tuple>;

template <typename ProTuple>
using pro_value_variant =
    transform_tuple<transform_tuple<ProTuple, promise_value, std::tuple>, wrap_void, std::variant>;

template <typename... Pros>
struct MergeState
{
    using promise_tuple = std::tuple<Pros...>;
    using result_tuple = pro_value_tuple<promise_tuple>;
    using result_variant = pro_value_variant<promise_tuple>;

    promise_tuple promises = {};
    result_tuple result = {};
    std::atomic<size_t> counter = 0;
};

template <typename Fulfill, typename State, size_t Index>
inline void merge_helper(std::shared_ptr<Fulfill> fulfill, std::shared_ptr<State> state)
{
    using element_type =
        promise_value<std::tuple_element_t<Index, typename State::promise_tuple>>::type;
    if constexpr (std::is_same_v<void, element_type>) {
        std::get<Index>(state->promises).then([=]() {
            std::get<Index>(state->result) = std::monostate {};
            fulfill->template hit<Index>(state);
        });
    }
    else {
        std::get<Index>(state->promises).then([=](element_type value) {
            std::get<Index>(state->result) = std::move(value);
            fulfill->template hit<Index>(state);
        });
    }
}

template <typename Fulfill, typename... Pros>
inline void merge(std::shared_ptr<Fulfill> fulfill, Pros... pros)
{
    using State = MergeState<Pros...>;
    auto state = std::make_shared<State>();
    state->promises = std::make_tuple(pros...);

    [=]<std::size_t... I>(std::index_sequence<I...>) {
        (merge_helper<Fulfill, State, I>(fulfill, state), ...);
    }(std::make_index_sequence<std::tuple_size_v<typename MergeState<Pros...>::promise_tuple>> {});
}

template <typename State>
struct merge_all_fulfill
{
    Promise<typename State::result_tuple> promise;

    template <size_t Index>
    void hit(std::shared_ptr<State> state)
    {
        if (++state->counter == std::tuple_size_v<typename State::promise_tuple>) {
            promise.resolve(std::move(state->result));
        }
    }
};

template <typename State>
struct merge_any_fulfill
{
    Promise<typename State::result_variant> promise;

    template <size_t Index>
    void hit(std::shared_ptr<State> state)
    {
        if (++state->counter == 1) {
            typename State::result_variant result;
            result.template emplace<Index>(std::move(std::get<Index>(state->result)));
            promise.resolve(std::move(result));
        }
    }
};

}

template <typename... Pros>
inline auto all(Pros... pros)
{
    auto fulfill = std::make_shared<details::merge_all_fulfill<details::MergeState<Pros...>>>();
    details::merge(fulfill, pros...);
    return fulfill->promise;
}

template <typename... Pros>
inline auto any(Pros... pros)
{
    auto fulfill = std::make_shared<details::merge_any_fulfill<details::MergeState<Pros...>>>();
    details::merge(fulfill, pros...);
    return fulfill->promise;
}

}
