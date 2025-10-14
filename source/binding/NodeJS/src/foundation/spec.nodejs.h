#pragma once

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

using EnvType = Napi::Env;
using CallbackInfo = Napi::CallbackInfo;
using RawCallback = std::function<ValueType(const CallbackInfo&)>;

inline void FreeValue(EnvType, ValueType)
{
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

inline ObjectRefType Persistent(EnvType, ConstValueType val)
{
    return Napi::Persistent(val.As<Napi::Object>());
}

inline ValueType MakeFunction(EnvType env, const char* name, [[maybe_unused]] int argc, RawCallback func)
{
    return Napi::Function::New(env, func, name);
}

inline void BindValue([[maybe_unused]] EnvType env, ConstObjectType object, const char* prop, ValueType value)
{
    object.DefineProperty(Napi::PropertyDescriptor::Value(prop, value, napi_enumerable));
}

inline void BindGetter(EnvType, ConstObjectType object, const char* prop, const char*, RawCallback func)
{
    object.DefineProperty(Napi::PropertyDescriptor::Accessor(prop, func, napi_enumerable));
}

inline ValueType CallMember(EnvType, ConstObjectType object, const char* prop, std::vector<ValueType> args)
{
    return object.Get(prop).As<Napi::Function>().Call(args);
}

inline void init(EnvType)
{
}

}
