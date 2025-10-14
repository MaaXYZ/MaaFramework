#pragma once

#include <exception>
#include <format>
#include <string>

#include "spec.h"

namespace maajs
{

struct ConvertFailed : public std::exception
{
    std::string error;

    ConvertFailed(std::string&& err = "")
        : error(err)
    {
    }

    virtual const char* what() const noexcept override { return error.c_str(); }
};

template <typename Type>
struct JSConvert
{
    static std::string name() = delete;
    static Type from_value(EnvType env, ConstValueType val) = delete;
    static ValueType to_value(EnvType env, const Type& val) = delete;
};

// msvc want this
template <>
struct JSConvert<void>
{
    static std::string name() { return "void"; }

    static ValueType to_value(EnvType env) { return MakeUndefined(env); }
};

template <>
struct JSConvert<std::monostate>
{
    static std::string name() { return "null"; }

    static std::monostate from_value(EnvType, ConstValueType val)
    {
        if (IsNull(val)) {
            return {};
        }
        throw ConvertFailed {};
    }

    static ValueType to_value(EnvType env, const std::monostate&) { return MakeNull(env); }
};

template <>
struct JSConvert<std::string>
{
    static std::string name() { return "string"; }

    static std::string from_value(EnvType env, ConstValueType val)
    {
        if (IsString(val)) {
            return GetString(env, val);
        }
        throw ConvertFailed {};
    }

    static ValueType to_value(EnvType env, const std::string& val) { return MakeString(env, val); }
};

template <>
struct JSConvert<uint64_t>
{
    static std::string name() { return "string<uint64_t>"; }

    static uint64_t from_value(EnvType env, ConstValueType val)
    {
        if (IsString(val)) {
            return std::stoull(GetString(env, val));
        }
        throw ConvertFailed {};
    }

    static ValueType to_value(EnvType env, const uint64_t& val) { return MakeString(env, std::to_string(val)); }
};

template <typename Type>
struct JSConvert<std::optional<Type>>
{
    static std::string name() { return std::format("{} | null", JSConvert<Type>::name()); }

    static std::optional<Type> from_value(EnvType env, ConstValueType val)
    {
        if (IsNull(val)) {
            return std::nullopt;
        }
        else {
            return JSConvert<Type>::from_value(env, val);
        }
    }

    static ValueType to_value(EnvType env, const std::optional<Type>& val)
    {
        if (val) {
            return MakeNull(env);
        }
        else {
            return JSConvert<Type>::to_value(env, *val);
        }
    }
};

}
