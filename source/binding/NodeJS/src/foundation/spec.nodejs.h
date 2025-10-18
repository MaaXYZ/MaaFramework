#pragma once

#include <format>
#include <functional>
#include <string>

#include <napi.h> // IWYU pragma: export

namespace maajs
{

using EnvType = Napi::Env;
using ValueType = Napi::Value;
using ObjectType = Napi::Object;
using BooleanType = Napi::Boolean;
using StringType = Napi::String;
using NumberType = Napi::Number;
using FunctionType = Napi::Function;
using ArrayType = Napi::Array;
using PromiseType = Napi::Promise;
using ArrayBufferType = Napi::ArrayBuffer;

using ObjectRefType = Napi::ObjectReference;
using FunctionRefType = Napi::FunctionReference;

using CallbackInfo = Napi::CallbackInfo;
using RawCallback = std::function<ValueType(const CallbackInfo&)>;

using NativeMarkerFunc = std::function<void(const maajs::ValueType&)>;

struct NativeClassBase
{
    virtual ~NativeClassBase() = default;

    virtual void gc_mark([[maybe_unused]] NativeMarkerFunc marker) {}
};

inline ValueType MakeArray(EnvType env, std::vector<ValueType> vals)
{
    auto arr = Napi::Array::New(env, vals.size());
    for (size_t i = 0; i < vals.size(); i++) {
        arr[static_cast<uint32_t>(i)] = vals[i];
    }
    return arr;
}

inline std::vector<ValueType> GetArray(ArrayType arr)
{
    auto len = arr.Length();
    std::vector<ValueType> result;
    result.reserve(len);
    for (uint32_t i = 0; i < len; i++) {
        result.push_back(arr[i]);
    }
    return result;
}

inline ObjectRefType PersistentObject(ObjectType val)
{
    return Napi::Persistent(val);
}

inline FunctionRefType PersistentFunction(FunctionType val)
{
    return Napi::Persistent(val);
}

inline ValueType ThrowTypeError(EnvType env, const std::string& err)
{
    Napi::TypeError::New(env, err).ThrowAsJavaScriptException();
    return env.Undefined();
}

inline FunctionType MakeFunction(
    EnvType env,
    const char* name,
    [[maybe_unused]] int argc,
    RawCallback func,
    std::function<void(NativeMarkerFunc)> = nullptr)
{
    return Napi::Function::New(env, func, name);
}

inline std::tuple<PromiseType, std::shared_ptr<FunctionRefType>, std::shared_ptr<FunctionRefType>> MakePromise(EnvType env)
{
    auto deferred = Napi::Promise::Deferred::New(env);

    auto resolveFunc = MakeFunction(env, "__deferred_resolve", 1, [deferred](const CallbackInfo& info) {
        deferred.Resolve(info[0]);
        return info.Env().Undefined();
    });
    auto resolvePtr = std::make_shared<maajs::FunctionRefType>(maajs::PersistentFunction(resolveFunc));

    auto rejectFunc = MakeFunction(env, "__deferred_reject", 1, [deferred](const CallbackInfo& info) {
        deferred.Reject(info[0]);
        return info.Env().Undefined();
    });
    auto rejectPtr = std::make_shared<maajs::FunctionRefType>(maajs::PersistentFunction(rejectFunc));

    return { deferred.Promise(), resolvePtr, rejectPtr };
}

inline void BindValue(ObjectType object, const char* prop, ValueType value)
{
    object.DefineProperty(Napi::PropertyDescriptor::Value(prop, value, napi_enumerable));
}

inline void BindGetter(ObjectType object, const char* prop, const char*, RawCallback func, std::function<void(NativeMarkerFunc)> = nullptr)
{
    object.DefineProperty(Napi::PropertyDescriptor::Accessor(prop, func, napi_enumerable));
}

inline ObjectType CallCtor(const FunctionRefType& ctor, std::vector<ValueType> args)
{
    std::vector<napi_value> rawArgs;
    rawArgs.reserve(args.size());
    for (auto arg : args) {
        rawArgs.push_back(arg);
    }
    return ctor.New(rawArgs);
}

inline ValueType CallMember(ObjectType object, const char* prop, std::vector<ValueType> args)
{
    return object[prop].AsValue().As<Napi::Function>().Call(object, args);
}

inline std::string TypeOf(ValueType val)
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

inline std::string DumpValue(ValueType val)
{
    if (val.IsExternal()) {
        return "[external]";
    }
    else {
        std::string descStr = val.ToString().Utf8Value();
        if (descStr.length() > 20) {
            descStr = descStr.substr(0, 17) + "...";
        }
        return std::format("{} [{}]", descStr, TypeOf(val));
    }
}

inline void init(EnvType)
{
}

}
