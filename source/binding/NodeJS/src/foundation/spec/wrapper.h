#pragma once

#include <format>
#include <string>
#include <vector>

#include "convert.h"
#include "traits.h"

namespace maajs
{

template <typename Args>
constexpr size_t OptionalCount()
{
    constexpr size_t arg_count = std::tuple_size_v<Args>;

    size_t opt_count = 0;
    bool met_required = false;
    [&opt_count, &met_required]<size_t... I>(std::index_sequence<I...>) {
        ((IsOptionalParam<std::tuple_element_t<arg_count - I - 1, Args>> ? (met_required ? 0 : opt_count++) : (met_required = true)), ...);
    }(std::make_index_sequence<arg_count>());

    return opt_count;
}

inline std::string DumpCallParams(const CallbackInfo& info)
{
    std::vector<std::string> parts;

    for (size_t i = 0; i < info.Length(); i++) {
        parts.push_back(TypeOf(info[i]));
    }

    return std::format("{}.({})", TypeOf(info.This()), JoinString(parts, ", "));
}

template <typename ArgsTuple, typename Self = void>
inline std::string DumpExpectNames()
{
    if constexpr (std::is_same_v<Self, void>) {
        return std::format("undefined.{}", DumpTypeNames<ArgsTuple>());
    }
    else {
        return std::format("object<{}>.{}", Self::name, DumpTypeNames<ArgsTuple>());
    }
}

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

template <typename ArgsTuple, typename Self = void>
inline ArgsTuple UnWrapArgs(const CallbackInfo& info, bool append_sig = true)
{
    constexpr size_t arg_count = std::tuple_size_v<ArgsTuple>;

    constexpr size_t opt_count = OptionalCount<ArgsTuple>();
    constexpr size_t min_count = arg_count - opt_count;

    if (info.Length() < min_count || info.Length() > arg_count) {
        std::string errStr = std::format("expect {} ~ {} arguments, got {}", min_count, arg_count, info.Length());
        if (append_sig) {
            errStr = std::format("{}\n    Sig: {}\n    Got: {}", errStr, DumpExpectNames<ArgsTuple, Self>(), DumpCallParams(info));
        }
        throw MaaError { errStr };
    }

    try {
        ArgsTuple params;

        if constexpr (arg_count > 0) {
            [&params, &info]<size_t... I>(std::index_sequence<I...>) {
                (((I < info.Length() ? std::get<I>(params) = JSConvert<std::tuple_element_t<I, ArgsTuple>>::from_value(info[I])
                                     : std::get<I>(params) = std::tuple_element_t<I, ArgsTuple> {})),
                 ...);
            }(std::make_index_sequence<arg_count>());
        }

        return params;
    }
    catch (const MaaError& err) {
        if (append_sig) {
            throw MaaError {
                std::format("{}\n    Sig: {}\n    Got: {}", err.what(), DumpExpectNames<ArgsTuple, Self>(), DumpCallParams(info)),
            };
        }
        else {
            throw;
        }
    }
}

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
                auto params = UnWrapArgs<Args, Self>(info, false);

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
                    if (!self) {
                        throw MaaError { "Cast this failed" };
                    }
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
                std::string what = std::format(
                    "maa.{}: {}\n    Sig: {}\n    Got: {}",
                    (const char*)name.data,
                    exc.what(),
                    DumpExpectNames<Args, Self>(),
                    DumpCallParams(info));
                return ThrowTypeError(info.Env(), what);
            }
        };
    }

    static ValueType makeValue(EnvType env) { return MakeFunction(env, name.data, arg_count, make()); }
};

}
