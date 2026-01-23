#pragma once

#include <format>

#include "../instric.forward.h"
#include "bridge.h"

namespace maajs
{

struct NativePointerHolder
{
    static JSClassID& getClassId(EnvType env) { return getClassId(env.Runtime()); }

    static JSClassID& getClassId(JSRuntime* rt) { return QuickJSRuntimeBridgeData::get(rt)->nativePointerHolderClassId; }

    static void init(EnvType env)
    {
        static JSClassDef classDef = {
            .class_name = "NativePointerHolder",
            .finalizer =
                +[](JSRuntime* rt, JSValueConst data) {
                    delete take<NativeClassBase>(rt, data);
                    JS_SetOpaque(data, nullptr);
                },
            .gc_mark =
                +[](JSRuntime* rt, JSValueConst data, JS_MarkFunc* func) {
                    take<NativeClassBase>(rt, data)->gc_mark([rt, func](const ValueType& value) {
                        func(rt, reinterpret_cast<JSGCObjectHeader*>(JS_VALUE_GET_OBJ(value.value)));
                    });
                },
            .call = nullptr,
            .exotic = nullptr,
        };

        auto& classId = getClassId(env);

        JS_NewClassID(env.Runtime(), &classId);
        JS_NewClass(env.Runtime(), classId, &classDef);
    }

    static JSValue make(EnvType env, NativeClassBase* pointer)
    {
        auto obj = JS_NewObjectClass(env, getClassId(env));
        JS_SetOpaque(obj, pointer);
        return obj;
    }

    template <typename Type>
    static Type* take(EnvType env, JSValueConst value)
    {
        return take<Type>(env.Runtime(), value);
    }

    template <typename Type>
    static Type* take(JSRuntime* rt, JSValueConst value)
    {
        auto ptr = static_cast<NativeClassBase*>(JS_GetOpaque(value, getClassId(rt)));
        return dynamic_cast<Type*>(ptr);
    }
};

inline ValueType MakeArray(EnvType env, std::vector<ValueType> vals)
{
    return ArrayType::Make(env, vals);
}

inline std::vector<ValueType> GetArray(const ArrayType& val)
{
    auto len = val.Length();
    std::vector<ValueType> result;
    result.reserve(len);
    for (uint32_t i = 0; i < len; i++) {
        result.push_back(val[i]);
    }
    return result;
}

inline ObjectRefType PersistentObject(ObjectType val)
{
    return { val };
}

inline FunctionRefType PersistentFunction(FunctionType val)
{
    return { val };
}

inline WeakObjectRefType WeakRefObject(ObjectType val)
{
    return WeakObjectRefType::Make(val);
}

inline ValueType ThrowTypeError(EnvType env, const std::string& err)
{
    JS_ThrowTypeError(env, "%s", err.c_str());
    return { env, JS_EXCEPTION };
}

inline FunctionType
    MakeFunction(EnvType env, const char* name, int argc, RawCallback func, std::function<void(NativeMarkerFunc)> run_marker)
{
    struct FuncHolder : public NativeClassBase
    {
        RawCallback func;
        std::function<void(NativeMarkerFunc)> run_marker;

        FuncHolder(RawCallback func, std::function<void(NativeMarkerFunc)> run_marker)
            : func(func)
            , run_marker(run_marker)
        {
        }

        void gc_mark(NativeMarkerFunc marker) override
        {
            if (run_marker) {
                run_marker(marker);
            }
        }
    };

    auto data = NativePointerHolder::make(
        env,
        new FuncHolder {
            func,
            run_marker,
        });
    auto result = JS_NewCFunctionData(
        env,
        +[](JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv, int, JSValueConst* func_data) -> JSValue {
            auto data = NativePointerHolder::take<FuncHolder>({ ctx }, func_data[0]);
            if (!data) {
                return JS_UNDEFINED;
            }
            CallbackInfo info {
                ctx,
                this_val,
                argc,
                argv,
            };
            return data->func(info).take();
        },
        argc,
        0,
        1,
        &data);
    JS_FreeValue(env, data);

    JS_DefinePropertyValue(env, result, 55 /* JS_ATOM_name */, JS_NewString(env, name), JS_PROP_CONFIGURABLE);

    return { env, result };
}

inline std::tuple<PromiseType, std::shared_ptr<FunctionRefType>, std::shared_ptr<FunctionRefType>> MakePromise(EnvType env)
{
    JSValue funcs[2];
    auto retPro = JS_NewPromiseCapability(env, funcs);

    auto resolvePtr = std::make_shared<FunctionRefType>(PersistentFunction({ env, funcs[0] }));

    auto rejectPtr = std::make_shared<FunctionRefType>(PersistentFunction({ env, funcs[1] }));

    return { { env, retPro }, resolvePtr, rejectPtr };
}

inline void BindValue(ObjectType object, const char* prop, ValueType value)
{
    JS_DefinePropertyValue(object.Env(), object.peek(), JS_NewAtom(object.Env(), prop), value.take(), JS_PROP_ENUMERABLE);
}

inline void BindGetterSetter(
    ObjectType object,
    const char* prop,
    const char* getter_name,
    const char* setter_name,
    RawCallback getter,
    RawCallback setter,
    std::function<void(NativeMarkerFunc)> run_getter_marker,
    std::function<void(NativeMarkerFunc)> run_setter_marker)
{
    if (!getter) {
        getter = [prop = std::string(prop)](const CallbackInfo& info) {
            return StringType::New(info.Env(), std::format("no getter for {}", prop));
        };
    }
    if (!setter) {
        setter = [prop = std::string(prop)](const CallbackInfo& info) {
            return info.Env().Undefined();
        };
    }
    JS_DefinePropertyGetSet(
        object.Env(),
        object.peek(),
        JS_NewAtom(object.Env(), prop),
        getter ? maajs::MakeFunction(object.Env(), getter_name, 0, getter, run_getter_marker).take() : JS_UNDEFINED,
        setter ? maajs::MakeFunction(object.Env(), setter_name, 1, setter, run_setter_marker).take() : JS_UNDEFINED,
        JS_PROP_ENUMERABLE);
}

inline ObjectType CallCtor(FunctionType ctor, std::vector<ValueType> args)
{
    auto rawArgs = QjsArray::__Trans(args, false);
    auto result = JS_CallConstructor(ctor.Env(), ctor.peek(), static_cast<int>(rawArgs.size()), rawArgs.data());
    return { ctor.Env(), result };
}

inline bool IsError(ValueType val)
{
    return JS_IsError(val.context, val.peek());
}

inline std::string ClassName(ObjectType val)
{
    return val["constructor"].AsValue().As<ObjectType>()["name"].AsValue().As<StringType>().Utf8Value();
}

inline std::string TypeOf(ValueType val)
{
    switch (val.value.tag) {
    case JS_TAG_UNDEFINED:
        return "undefined";
    case JS_TAG_NULL:
        return "null";
    case JS_TAG_BOOL:
        return "boolean";
    case JS_TAG_INT:
    case JS_TAG_FLOAT64:
        return "number";
    case JS_TAG_STRING:
        return "string";
    case JS_TAG_SYMBOL:
        return "symbol";
    case JS_TAG_OBJECT:
        if (IsError(val)) {
            return std::format("error[{}]", ClassName(val.As<ObjectType>()));
        }
        else if (val.IsFunction()) {
            return "function";
        }
        else if (val.IsArray()) {
            return "array";
        }
        else if (val.IsArrayBuffer()) {
            return "arraybuffer";
        }
        else {
            return std::format("object[{}]", ClassName(val.As<ObjectType>()));
        }
    case JS_TAG_BIG_INT:
    case JS_TAG_SHORT_BIG_INT:
        return "bigint";
    case JS_TAG_EXCEPTION:
        return "exception";
    }
    return "unknown";
}

inline std::string DumpValue(ValueType val)
{
    if (IsError(val)) {
        return val.As<ObjectType>()["message"].AsValue().As<StringType>().Utf8Value();
    }

    std::string descStr = val.ToString().Utf8Value();
    if (descStr.length() > 50) {
        descStr = descStr.substr(0, 47) + "...";
    }
    return std::format("{} [{}]", descStr, TypeOf(val));
}

inline void init(EnvType env)
{
    NativePointerHolder::init(env);
}

}
