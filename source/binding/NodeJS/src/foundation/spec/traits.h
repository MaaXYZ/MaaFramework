#pragma once

#include <tuple>

#include "types.h"

namespace maajs
{

template <typename F>
struct FuncTraits
{
};

template <typename R, typename... Args>
struct FuncTraits<R (*)(Args...)>
{
    using ret = R;
    using args = std::tuple<Args...>;
    using call_args = args;
    using self = void;
    static constexpr bool want_self = false;
    static constexpr bool want_self_value = false;
    static constexpr bool want_env = false;
};

template <typename R, typename... Args>
struct FuncTraits<R (*)(EnvType, Args...)>
{
    using ret = R;
    using args = std::tuple<Args...>;
    using call_args = std::tuple<EnvType, Args...>;
    using self = void;
    static constexpr bool want_self = false;
    static constexpr bool want_self_value = false;
    static constexpr bool want_env = true;
};

template <typename R, typename T, typename... Args>
struct FuncTraits<R (T::*)(Args...)>
{
    using ret = R;
    using args = std::tuple<Args...>;
    using call_args = args;
    using self = T;
    static constexpr bool want_self = true;
    static constexpr bool want_self_value = false;
    static constexpr bool want_env = false;
};

template <typename R, typename T, typename... Args>
struct FuncTraits<R (T::*)(EnvType, Args...)>
{
    using ret = R;
    using args = std::tuple<Args...>;
    using call_args = std::tuple<EnvType, Args...>;
    using self = T;
    static constexpr bool want_self = true;
    static constexpr bool want_self_value = false;
    static constexpr bool want_env = true;
};

template <typename R, typename T, typename... Args>
struct FuncTraits<R (T::*)(ValueType, EnvType, Args...)>
{
    using ret = R;
    using args = std::tuple<Args...>;
    using call_args = std::tuple<ValueType, EnvType, Args...>;
    using self = T;
    static constexpr bool want_self = true;
    static constexpr bool want_self_value = true;
    static constexpr bool want_env = true;
};

}
