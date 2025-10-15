#pragma once

#include <format>
#include <functional>
#include <string>

#include <napi.h> // IWYU pragma: export

namespace maajs
{

using ValueType = Napi::Value;
using ConstValueType = Napi::Value;
using ObjectType = Napi::Object;
using ConstObjectType = Napi::Object;
using ObjectRefType = Napi::ObjectReference;
using FunctionRefType = Napi::FunctionReference;
using PromiseType = Napi::Promise;

using EnvType = Napi::Env;
using CallbackInfo = Napi::CallbackInfo;
using RawCallback = std::function<ValueType(const CallbackInfo&)>;

using NativeMarkerFunc = std::function<void(maajs::ConstValueType)>;

struct NativeClassBase
{
    virtual ~NativeClassBase() = default;

    virtual void gc_mark([[maybe_unused]] NativeMarkerFunc marker) {}
};

inline ValueType DupValue(EnvType, ConstValueType val)
{
    return val;
}

inline void FreeValue(EnvType, ValueType)
{
}

inline ValueType ObjectToValue(ObjectType val)
{
    return val.As<Napi::Value>();
}

inline ObjectType ValueToObject(ValueType val)
{
    return val.As<Napi::Object>();
}

inline ValueType MakeNull(EnvType env)
{
    return env.Null();
}

inline bool IsNull(ConstValueType val)
{
    return val.IsNull();
}

inline ValueType MakeUndefined(EnvType env)
{
    return env.Undefined();
}

inline bool IsUndefined(ConstValueType val)
{
    return val.IsUndefined();
}

inline ValueType MakeNumber(EnvType env, int32_t val)
{
    return Napi::Number::New(env, val);
}

inline ValueType MakeNumber(EnvType env, double val)
{
    return Napi::Number::New(env, val);
}

inline bool IsNumber(ConstValueType val)
{
    return val.IsNumber();
}

inline int32_t GetNumberI32(ConstValueType val)
{
    return val.As<Napi::Number>().Int32Value();
}

inline ObjectType MakeObject(EnvType env)
{
    return Napi::Object::New(env);
}

inline ValueType MakeString(EnvType env, std::string value)
{
    return Napi::String::New(env, value);
}

inline bool IsString(ConstValueType val)
{
    return val.IsString();
}

inline std::string GetString(EnvType, ConstValueType val)
{
    return val.As<Napi::String>().Utf8Value();
}

inline ObjectRefType PersistentObject(EnvType, ConstValueType val)
{
    return Napi::Persistent(val.As<Napi::Object>());
}

inline FunctionRefType PersistentFunction(EnvType, ConstValueType val)
{
    return Napi::Persistent(val.As<Napi::Function>());
}

inline ValueType MakeFunction(
    EnvType env,
    const char* name,
    [[maybe_unused]] int argc,
    RawCallback func,
    std::function<void(NativeMarkerFunc)> = nullptr)
{
    return Napi::Function::New(env, func, name);
}

inline void BindValue([[maybe_unused]] EnvType env, ConstObjectType object, const char* prop, ValueType value)
{
    object.DefineProperty(Napi::PropertyDescriptor::Value(prop, value, napi_enumerable));
}

inline void BindGetter(
    EnvType,
    ConstObjectType object,
    const char* prop,
    const char*,
    RawCallback func,
    std::function<void(NativeMarkerFunc)> = nullptr)
{
    object.DefineProperty(Napi::PropertyDescriptor::Accessor(prop, func, napi_enumerable));
}

inline ValueType CallCtor(EnvType, const FunctionRefType& ctor, std::vector<ValueType> args)
{
    std::vector<napi_value> rawArgs(args.size());
    for (auto arg : args) {
        rawArgs.push_back(arg);
    }
    return ctor.New(rawArgs);
}

inline ValueType CallMember(EnvType, ConstObjectType object, const char* prop, std::vector<ValueType> args)
{
    return object.Get(prop).As<Napi::Function>().Call(args);
}

inline std::string_view TypeOf(EnvType, ConstValueType val)
{
    switch (val.Type()) {
    case napi_undefined:
        return "undefined";
    case napi_null:
        return "null";
    case napi_boolean:
        return "boolean";
    case napi_number:
        return "number";
    case napi_string:
        return "string";
    case napi_symbol:
        return "symbol";
    case napi_object:
        return "object";
    case napi_function:
        return "function";
    case napi_external:
        return "external";
    case napi_bigint:
        return "bigint";
    }
    return "unknown";
}

inline std::string DumpValue(EnvType env, ConstValueType val)
{
    if (val.IsExternal()) {
        return "[external]";
    }
    else {
        std::string descStr = val.ToString().Utf8Value();
        if (descStr.length() > 20) {
            descStr = descStr.substr(0, 17) + "...";
        }
        return std::format("{} [{}]", descStr, TypeOf(env, val));
    }
}

inline void init(EnvType)
{
}

}
