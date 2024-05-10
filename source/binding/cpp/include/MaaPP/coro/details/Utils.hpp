// IWYU pragma: private, include "MaaPP/MaaPP.h"

#pragma once

#include "MaaPP/module/Module.h"

#ifndef MAAPP_USE_MODULE

#include <tuple>
#include <variant>

#endif

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

MAAPP_EXPORT template <
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

MAAPP_EXPORT template <typename P>
struct promise_value
{
    using type = typename P::value_t;
};

MAAPP_EXPORT template <typename T>
struct wrap_void
{
    using type = T;
};

MAAPP_EXPORT template <>
struct wrap_void<void>
{
    using type = std::monostate;
};

}
