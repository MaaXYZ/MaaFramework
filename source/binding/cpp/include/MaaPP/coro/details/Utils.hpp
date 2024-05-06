// IWYU pragma: private, include "MaaPP/MaaPP.h"

#pragma once

#include <tuple>
#include <variant>

namespace maa::coro::details
{

template <
    typename Tuple,
    template <typename>
    class Transformer,
    template <typename...>
    class Container,
    typename Seq>
struct transform_tuple_impl;

template <
    typename... Types,
    template <typename>
    class Transformer,
    template <typename...>
    class Container,
    std::size_t... I>
struct transform_tuple_impl<std::tuple<Types...>, Transformer, Container, std::index_sequence<I...>>
{
    using type = Container<typename Transformer<Types>::type...>;
};

template <
    typename Tuple,
    template <typename>
    class Transformer,
    template <typename...>
    class Container>
using transform_tuple = typename transform_tuple_impl<
    Tuple,
    Transformer,
    Container,
    std::make_index_sequence<std::tuple_size_v<Tuple>>>::type;

template <typename P>
struct promise_value
{
    using type = typename P::value_t;
};

template <typename T>
struct wrap_void
{
    using type = T;
};

template <>
struct wrap_void<void>
{
    using type = std::monostate;
};

}
