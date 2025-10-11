#pragma once

#include <exception>
#include <format>
#include <string>

#include "spec.h"
#include "spec.quickjs.h"
#include "traits.h"

namespace maa_js
{

struct ConvertFailed : public std::exception
{
    std::string error;

    ConvertFailed(std::string&& err)
        : error(err)
    {
    }

    virtual const char* what() const noexcept override { return error.c_str(); }
};

template <typename Type>
struct JSConvert
{
    static std::string name() = delete;
    static Type from_value(ValueType val) = delete;
    static ValueType to_value(ContextType env, const Type& val) = delete;
};

// msvc want this
template <>
struct JSConvert<void>
{
    static std::string name() { return "void"; }

    static ValueType to_value([[maybe_unused]] ContextType env) { return get_undefined(env); }
};

template <typename Type, void (*Finalizer)(ContextType, Type*)>
struct JSConvert<ExternalType<Type, Finalizer>>
{
    static std::string name() { return ExternalTraits<Type>::name; }

    static Type from_value(ValueType val)
    {
        if (!ExternalType<Type, Finalizer>::check(val)) {
            throw ConvertFailed { std::format("expect {}, got {}", name(), DumpValue(val)) };
        }
    };
};

}
