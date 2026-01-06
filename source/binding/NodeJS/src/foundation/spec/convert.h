#pragma once

#include <array>
#include <format>
#include <optional>
#include <string>
#include <tuple>
#include <variant>

#include "class.h"
#include "tools.h"

namespace maajs
{

template <typename T>
struct OptionalParam : public std::optional<T>
{
    using std::optional<T>::optional;
};

template <typename T>
constexpr bool IsOptionalParam = false;

template <typename T>
constexpr bool IsOptionalParam<OptionalParam<T>> = true;

template <typename Inherit>
struct NativeObject
{
    Inherit* impl {};

    operator Inherit*() const { return impl; }

    Inherit* operator->() const { return impl; }
};

template <typename T>
constexpr bool IsNativeObject = false;

template <typename T>
constexpr bool IsNativeObject<NativeObject<T>> = true;

template <typename Type>
struct JSConvert
{
    static std::string name() = delete;
    static Type from_value(ValueType val) = delete;
    static ValueType to_value(EnvType env, const Type& val) = delete;
};

template <typename T>
constexpr bool IsTuple = false;

template <typename... Args>
constexpr bool IsTuple<std::tuple<Args...>> = true;

template <typename T>
constexpr bool IsVariant = false;

template <typename... Args>
constexpr bool IsVariant<std::variant<Args...>> = true;

template <typename ArgsTuple>
inline std::string DumpTypeNames(std::string sep = ", ")
{
    std::vector<std::string> parts;

    if constexpr (IsTuple<ArgsTuple>) {
        [&]<size_t... I>(std::index_sequence<I...>) {
            ((parts.push_back(JSConvert<std::tuple_element_t<I, ArgsTuple>>::name())), ...);
        }(std::make_index_sequence<std::tuple_size_v<ArgsTuple>>());
    }
    else if constexpr (IsVariant<ArgsTuple>) {
        [&]<size_t... I>(std::index_sequence<I...>) {
            ((parts.push_back(JSConvert<std::variant_alternative_t<I, ArgsTuple>>::name())), ...);
        }(std::make_index_sequence<std::variant_size_v<ArgsTuple>>());
    }

    return JoinString(parts, sep);
}

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
    static std::string name() { return "unknown"; };

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
struct JSConvert<ArrayBufferType>
{
    static std::string name() { return "ArrayBuffer"; };

    static ArrayBufferType from_value(ValueType val)
    {
        if (val.IsArrayBuffer()) {
            return val.As<ArrayBufferType>();
        }
        throw MaaError { std::format("expect {}, got {}", name(), DumpValue(val)) };
    }

    static ValueType to_value(EnvType, const ArrayBufferType& val) { return val; }
};

template <>
struct JSConvert<nullptr_t>
{
    static std::string name() { return "null"; }

    static nullptr_t from_value(ValueType val)
    {
        if (val.IsNull()) {
            return {};
        }
        throw MaaError { std::format("expect {}, got {}", name(), DumpValue(val)) };
    }

    static ValueType to_value(EnvType env, const nullptr_t&) { return env.Null(); }
};

template <>
struct JSConvert<std::monostate>
{
    static std::string name() { return "undefined"; }

    static std::monostate from_value(ValueType val)
    {
        if (val.IsUndefined()) {
            return {};
        }
        throw MaaError { std::format("expect {}, got {}", name(), DumpValue(val)) };
    }

    static ValueType to_value(EnvType env, const std::monostate&) { return env.Undefined(); }
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

#ifdef __APPLE__
// mac上uintptr_t是unsigned long, uint64_t是unsigned long long
template <>
struct JSConvert<uintptr_t>
{
    static std::string name() { return "string<uint64_t>"; }

    static uintptr_t from_value(ValueType val)
    {
        if (val.IsString()) {
            return std::stoull(val.As<StringType>().Utf8Value());
        }
        throw MaaError { std::format("expect {}, got {}", name(), DumpValue(val)) };
    }

    static ValueType to_value(EnvType env, const uintptr_t& val) { return StringType::New(env, std::to_string(val)); }
};
#else
static_assert(std::is_same_v<uint64_t, uintptr_t>, "");
#endif

template <typename Type>
struct JSConvert<std::vector<Type>>
{
    static std::string name() { return std::format("{}[]", JSConvert<Type>::name()); }

    static std::vector<Type> from_value(ValueType val)
    {
        if (val.IsArray()) {
            auto vals = GetArray(val.As<ArrayType>());
            std::vector<Type> result;
            for (const auto& v : vals) {
                result.push_back(JSConvert<Type>::from_value(v));
            }
            return result;
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

template <typename Type, size_t N>
struct JSConvert<std::array<Type, N>>
{
    static std::string name() { return std::format("{}[{}]", JSConvert<Type>::name(), N); }

    static std::array<Type, N> from_value(ValueType val)
    {
        if (val.IsArray() && val.As<ArrayType>().Length() == N) {
            auto vals = GetArray(val.As<ArrayType>());
            std::array<Type, N> result;
            for (size_t i = 0; i < N; i++) {
                result[i] = JSConvert<Type>::from_value(vals[i]);
            }
            return result;
        }
        throw MaaError { std::format("expect {}, got {}", name(), DumpValue(val)) };
    }

    static ValueType to_value(EnvType env, const std::array<Type, N>& vals)
    {
        std::vector<ValueType> result;
        result.reserve(N);
        for (const auto& val : vals) {
            result.push_back(JSConvert<Type>::to_value(env, val));
        }
        return MakeArray(env, result);
    }
};

template <typename... Args>
struct JSConvert<std::tuple<Args...>>
{
    using T = std::tuple<Args...>;

    static std::string name() { return std::format("[{}]", DumpTypeNames<T>()); }

    static T from_value(ValueType val)
    {
        if (val.IsArray()) {
            auto arr = val.As<ArrayType>();
            if (arr.Length() != std::tuple_size_v<T>) {
                throw MaaError { std::format("expect {}, got {}", name(), DumpValue(val)) };
            }
            T result;
            [&]<size_t... I>(std::index_sequence<I...>) {
                ((std::get<I>(result) = JSConvert<std::tuple_element_t<I, T>>::from_value(arr[I])), ...);
            }(std::make_index_sequence<std::tuple_size_v<T>>());
            return result;
        }
        throw MaaError { std::format("expect {}, got {}", name(), DumpValue(val)) };
    }

    static ValueType to_value(EnvType env, const T& val)
    {
        std::vector<ValueType> arr;
        [&]<size_t... I>(std::index_sequence<I...>) {
            ((arr.push_back(JSConvert<std::tuple_element_t<I, T>>::to_value(env, std::get<I>(val)))), ...);
        }(std::make_index_sequence<std::tuple_size_v<T>>());
        return MakeArray(env, arr);
    }
};

template <typename... Args>
struct JSConvert<std::variant<Args...>>
{
    using T = std::variant<Args...>;

    static std::string name() { return std::format("{}", DumpTypeNames<T>(" | ")); }

    template <size_t I>
    static bool try_from_value(T& result, ValueType val)
    {
        using Type = std::variant_alternative_t<I, T>;
        try {
            result = JSConvert<Type>::from_value(val);
            return true;
        }
        catch (const MaaError&) {
            return false;
        }
    }

    static T from_value(ValueType val)
    {
        T result;
        auto succ = [&]<size_t... I>(std::index_sequence<I...>) {
            return (try_from_value<I>(result, val) || ...);
        }(std::make_index_sequence<std::variant_size_v<T>>());
        if (succ) {
            return result;
        }
        throw MaaError { std::format("expect {}, got {}", name(), DumpValue(val)) };
    }

    template <size_t I>
    static bool try_to_value(ValueType& result, EnvType env, const T& val)
    {
        using Type = std::variant_alternative_t<I, T>;
        if (auto ptr = std::get_if<Type>(&val)) {
            result = JSConvert<Type>::to_value(env, *ptr);
            return true;
        }
        else {
            return false;
        }
    }

    static ValueType to_value(EnvType env, const T& val)
    {
        ValueType result;
        [&]<size_t... I>(std::index_sequence<I...>) {
            std::ignore = (try_to_value<I>(result, env, val) || ...);
        }(std::make_index_sequence<std::variant_size_v<T>>());
        return result;
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

template <typename Type>
struct JSConvert<OptionalParam<Type>>
{
    static std::string name() { return std::format("?{}", JSConvert<Type>::name()); }

    static OptionalParam<Type> from_value(ValueType val)
    {
        if (val.IsUndefined()) {
            return std::nullopt;
        }
        else {
            return JSConvert<Type>::from_value(val);
        }
    }

    static ValueType to_value(EnvType env, const OptionalParam<Type>& val)
    {
        if (val) {
            return JSConvert<Type>::to_value(env, *val);
        }
        else {
            return env.Undefined();
        }
    }
};

template <typename Inherit>
struct JSConvert<NativeObject<Inherit>>
{
    static std::string name() { return std::format("object<{}>", Inherit::name); }

    static NativeObject<Inherit> from_value(ValueType val)
    {
        if (val.IsObject()) {
            auto impl = NativeClass<Inherit>::take(val);
            if (impl) {
                return { impl };
            }
        }
        throw MaaError { std::format("expect {}, got {}", name(), DumpValue(val)) };
    }

    static ValueType to_value(EnvType env, const NativeObject<Inherit>&) = delete;
};

}
