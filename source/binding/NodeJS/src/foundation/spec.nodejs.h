#pragma once

#include <napi.h>

namespace maa_js
{

using ValueType = Napi::Value;
using ContextType = Napi::Env;

inline ValueType get_undefined(ContextType env)
{
    return env.Undefined();
}

template <typename Type, void (*Finalizer)(ContextType, Type*)>
struct ExternalType
{
    Napi::External<Type> _impl;

    static void init(ContextType) {}

    static ExternalType create(ContextType env, Type* data) { return { Napi::External<Type>::New(env, data, Finalizer) }; }

    static ExternalType cast(ValueType value) { return { value }; }

    static bool check(ValueType value) { return value.IsExternal(); }

    Type* get() const { return _impl.Data(); }
};

}
