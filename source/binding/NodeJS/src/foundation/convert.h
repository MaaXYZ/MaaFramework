#pragma once

#include <exception>
#include <format>
#include <optional>
#include <string>
#include <variant>

#include "classes.h"
#include "spec.h"

namespace maajs
{

struct MaaError : public std::exception
{
    std::string error;

    MaaError(std::string&& err = "")
        : error(err)
    {
    }

    virtual const char* what() const noexcept override { return error.c_str(); }
};

template <size_t N>
struct StringHolder
{
    char data[N];

    constexpr StringHolder(const char (&str)[N]) { std::copy(str, str + N, data); }
};

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

template <typename Type>
struct JSConvert
{
    static std::string name() = delete;
    static Type from_value(ValueType val) = delete;
    static ValueType to_value(EnvType env, const Type& val) = delete;
};

// msvc want this
template <>
struct JSConvert<void>
{
    static std::string name() { return "void"; }

    static ValueType to_value(EnvType env) { return env.Undefined(); }
};

template <>
struct JSConvert<ValueType>
{
    static std::string name() = delete;

    static ValueType from_value(ValueType val) { return val; }

    static ValueType to_value(EnvType, const ValueType& val) { return val; }
};

template <>
struct JSConvert<ObjectType>
{
    static std::string name() { return "object"; };

    static ObjectType from_value(ValueType val)
    {
        if (val.IsObject()) {
            return val.As<ObjectType>();
        }
        throw MaaError { std::format("expect {}, got {}", name(), DumpValue(val)) };
    }

    static ValueType to_value(EnvType, const ObjectType& val) { return val; }
};

template <>
struct JSConvert<FunctionType>
{
    static std::string name() { return "function"; };

    static FunctionType from_value(ValueType val)
    {
        if (val.IsFunction()) {
            return val.As<FunctionType>();
        }
        throw MaaError { std::format("expect {}, got {}", name(), DumpValue(val)) };
    }

    static ValueType to_value(EnvType, const FunctionType& val) { return val; }
};

template <>
struct JSConvert<PromiseType>
{
    static std::string name() { return "promise"; };

    static PromiseType from_value(ValueType val)
    {
        if (val.IsPromise()) {
            return val.As<PromiseType>();
        }
        throw MaaError { std::format("expect {}, got {}", name(), DumpValue(val)) };
    }

    static ValueType to_value(EnvType, const PromiseType& val) { return val; }
};

template <>
struct JSConvert<std::monostate>
{
    static std::string name() { return "null"; }

    static std::monostate from_value(ValueType val)
    {
        if (val.IsNull()) {
            return {};
        }
        throw MaaError { std::format("expect {}, got {}", name(), DumpValue(val)) };
    }

    static ValueType to_value(EnvType env, const std::monostate&) { return env.Null(); }
};

template <>
struct JSConvert<bool>
{
    static std::string name() { return "boolean"; }

    static bool from_value(ValueType val)
    {
        if (val.IsBoolean()) {
            return val.As<BooleanType>().Value();
        }
        throw MaaError { std::format("expect {}, got {}", name(), DumpValue(val)) };
    }

    static ValueType to_value(EnvType env, const bool& val) { return BooleanType::New(env, val); }
};

template <>
struct JSConvert<std::string>
{
    static std::string name() { return "string"; }

    static std::string from_value(ValueType val)
    {
        if (val.IsString()) {
            return val.As<StringType>().Utf8Value();
        }
        throw MaaError { std::format("expect {}, got {}", name(), DumpValue(val)) };
    }

    static ValueType to_value(EnvType env, const std::string& val) { return StringType::New(env, val); }
};

template <>
struct JSConvert<int32_t>
{
    static std::string name() { return "number<int32_t>"; }

    static int32_t from_value(ValueType val)
    {
        if (val.IsNumber()) {
            return val.As<NumberType>().Int32Value();
        }
        throw MaaError { std::format("expect {}, got {}", name(), DumpValue(val)) };
    }

    static ValueType to_value(EnvType env, const int32_t& val) { return NumberType::New(env, val); }
};

template <>
struct JSConvert<int64_t>
{
    static std::string name() { return "string<int64_t>"; }

    static int64_t from_value(ValueType val)
    {
        if (val.IsString()) {
            return std::stoll(val.As<StringType>().Utf8Value());
        }
        throw MaaError { std::format("expect {}, got {}", name(), DumpValue(val)) };
    }

    static ValueType to_value(EnvType env, const int64_t& val) { return StringType::New(env, std::to_string(val)); }
};

template <>
struct JSConvert<uint64_t>
{
    static std::string name() { return "string<uint64_t>"; }

    static uint64_t from_value(ValueType val)
    {
        if (val.IsString()) {
            return std::stoull(val.As<StringType>().Utf8Value());
        }
        throw MaaError { std::format("expect {}, got {}", name(), DumpValue(val)) };
    }

    static ValueType to_value(EnvType env, const uint64_t& val) { return StringType::New(env, std::to_string(val)); }
};

template <typename Type>
struct JSConvert<std::vector<Type>>
{
    static std::string name() { return std::format("{}[]", JSConvert<Type>::name()); }

    static std::vector<Type> from_value(ValueType val)
    {
        if (val.IsArray()) {
            auto vals = GetArray(val.As<ArrayType>());
            std::vector<Type> result;
            for (const auto& val : vals) {
                result.push_back(JSConvert<Type>::from_value(val));
            }
        }
        throw MaaError { std::format("expect {}, got {}", name(), DumpValue(val)) };
    }

    static ValueType to_value(EnvType env, const std::vector<Type>& vals)
    {
        std::vector<ValueType> result;
        result.reserve(vals.size());
        for (const auto& val : vals) {
            result.push_back(JSConvert<Type>::to_value(env, val));
        }
        return MakeArray(env, result);
    }
};

template <typename Type>
struct JSConvert<std::optional<Type>>
{
    static std::string name() { return std::format("{} | null", JSConvert<Type>::name()); }

    static std::optional<Type> from_value(ValueType val)
    {
        if (val.IsNull()) {
            return std::nullopt;
        }
        else {
            return JSConvert<Type>::from_value(val);
        }
    }

    static ValueType to_value(EnvType env, const std::optional<Type>& val)
    {
        if (val) {
            return JSConvert<Type>::to_value(env, *val);
        }
        else {
            return env.Null();
        }
    }
};

template <typename Func, Func func, StringHolder name>
struct WrapFunctionHelper
{
    using traits = FuncTraits<Func>;

    constexpr static size_t arg_count = std::tuple_size_v<typename traits::args>;
    using Args = typename traits::args;
    using Ret = typename traits::ret;
    using Self = typename traits::self;
    using CallArgs = typename traits::call_args;

    static auto make()
    {
        return +[](const CallbackInfo& info) {
            try {
                if (info.Length() != arg_count) {
                    throw MaaError { std::format("expect {} arguments, got {}", arg_count, info.Length()) };
                }

                typename traits::args params;

                if constexpr (arg_count > 0) {
                    [&params, &info]<size_t... I>(std::index_sequence<I...>) {
                        ((std::get<I>(params) = JSConvert<std::tuple_element_t<I, Args>>::from_value(info[I])), ...);
                    }(std::make_index_sequence<arg_count>());
                }

                ValueType ret = info.Env().Undefined();

                CallArgs call_params = [p = std::move(params), &info]() {
                    if constexpr (traits::want_env) {
                        if constexpr (traits::want_self_value) {
                            return std::tuple_cat(std::make_tuple(info.This(), info.Env()), std::move(p));
                        }
                        else {
                            return std::tuple_cat(std::make_tuple(info.Env()), std::move(p));
                        }
                    }
                    else {
                        std::ignore = info;
                        return p;
                    }
                }();

                Self* self = nullptr;
                if constexpr (traits::want_self) {
                    self = NativeClass<Self>::take(info.This());
                }

                if constexpr (std::is_same_v<Ret, void>) {
                    if constexpr (traits::want_self) {
                        std::apply(std::bind_front(func, self), std::move(call_params));
                    }
                    else {
                        std::apply(func, std::move(call_params));
                    }
                    ret = info.Env().Undefined();
                }
                else {
                    Ret retVal;
                    if constexpr (traits::want_self) {
                        retVal = std::apply(std::bind_front(func, self), std::move(call_params));
                    }
                    else {
                        retVal = std::apply(func, std::move(call_params));
                    }
                    if constexpr (!std::is_same_v<Ret, ValueType>) {
                        ret = JSConvert<Ret>::to_value(info.Env(), retVal);
                    }
                    else {
                        ret = retVal;
                    }
                }

                return ret;
            }
            catch (const MaaError& exc) {
                std::string what = std::format("maa.{}: {}", (const char*)name.data, exc.what());
#ifdef MAA_JS_IMPL_IS_NODEJS
                Napi::TypeError::New(info.Env(), what).ThrowAsJavaScriptException();
                return info.Env().Undefined();
#endif
#ifdef MAA_JS_IMPL_IS_QUICKJS
                JS_ThrowTypeError(info.Env(), "%s", what.c_str());
                return QjsValue { info.Env(), JS_EXCEPTION };
#endif
            }
        };
    }

    static ValueType makeValue(EnvType env) { return MakeFunction(env, name.data, arg_count, make()); }
};

template <typename... Args>
inline std::vector<ValueType> WrapArgs(EnvType env, Args&&... args)
{
    using ArgsTuple = std::tuple<Args...>;

    ArgsTuple arg_tuple = ArgsTuple(std::forward<Args>(args)...);
    std::vector<ValueType> params;

    [&params, &arg_tuple, &env]<size_t... I>(std::index_sequence<I...>) {
        ((params.push_back(JSConvert<std::remove_cvref_t<std::tuple_element_t<I, ArgsTuple>>>::to_value(env, std::get<I>(arg_tuple)))),
         ...);
    }(std::make_index_sequence<std::tuple_size_v<ArgsTuple>>());

    return params;
}

template <typename... Args>
inline ValueType CallCtorHelper(const FunctionRefType& ctor, Args&&... args)
{
    auto params = WrapArgs(ctor.Env(), std::forward<Args>(args)...);

    auto val = CallCtor(ctor, params);

    return val;
}

template <typename Ret, typename... Args>
inline Ret CallFuncHelper(FunctionType func, Args&&... args)
{
    auto params = WrapArgs(func.Env(), std::forward<Args>(args)...);

    auto val = func.Call(params);

    if constexpr (!std::is_same_v<Ret, void>) {
        return JSConvert<Ret>::from_value(val);
    }
}

template <typename Ret, typename... Args>
inline Ret CallMemberHelper(ObjectType object, const char* prop, Args&&... args)
{
    auto params = WrapArgs(object.Env(), std::forward<Args>(args)...);

    auto val = CallMember(object, prop, params);

    if constexpr (!std::is_same_v<Ret, void>) {
        return JSConvert<Ret>::from_value(val);
    }
}

}
