#pragma once

#include <format>

#include "../instric.forward.h"

namespace maajs
{

inline ValueType MakeArray(EnvType env, std::vector<ValueType> vals)
{
    auto arr = Napi::Array::New(env, vals.size());
    for (size_t i = 0; i < vals.size(); i++) {
        arr[static_cast<uint32_t>(i)] = vals[i];
    }
    return arr;
}

inline std::vector<ValueType> GetArray(const ArrayType& arr)
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

inline WeakObjectRefType WeakRefObject(ObjectType val)
{
    return Napi::Weak(val);
}

inline ValueType ThrowTypeError(EnvType env, const std::string& err)
{
    Napi::TypeError::New(env, err).ThrowAsJavaScriptException();
    return env.Undefined();
}

inline FunctionType
    MakeFunction(EnvType env, const char* name, [[maybe_unused]] int argc, RawCallback func, std::function<void(NativeMarkerFunc)>)
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

inline void BindGetterSetter(
    ObjectType object,
    const char* prop,
    const char*,
    const char*,
    RawCallback getter,
    RawCallback setter,
    std::function<void(NativeMarkerFunc)>,
    std::function<void(NativeMarkerFunc)>)
{
    object.DefineProperty(
        Napi::PropertyDescriptor::Accessor(prop, getter, [setter](const CallbackInfo& info) { setter(info); }, napi_enumerable));
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
