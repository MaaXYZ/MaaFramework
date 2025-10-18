#pragma once

#include <format>
#include <functional>
#include <string>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4244)
#endif
#include <quickjs.h> // IWYU pragma: export
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include "spec.quickjs.wrapper.h"

namespace maajs
{

using EnvType = QjsEnv;
using ValueType = QjsValue;
using ObjectType = QjsObject;
using BooleanType = QjsBoolean;
using StringType = QjsString;
using NumberType = QjsNumber;
using FunctionType = QjsFunction;
using ArrayType = QjsArray;
using PromiseType = QjsPromise;
using ArrayBufferType = QjsArrayBuffer;

using ObjectRefType = QjsRef<ObjectType>;
using FunctionRefType = QjsRef<FunctionType>;

using CallbackInfo = QjsCallbackInfo;
using RawCallback = std::function<ValueType(const CallbackInfo&)>;

using NativeMarkerFunc = std::function<void(const ValueType& value)>;

struct NativeClassBase
{
    virtual ~NativeClassBase() = default;

    virtual void gc_mark([[maybe_unused]] NativeMarkerFunc marker) {}
};

struct NativePointerHolder
{
    static JSClassID _classId;

    static void init(EnvType env)
    {
        static JSClassDef classDef = {
            .class_name = "__MaaNativePointerHolder",
            .finalizer =
                +[](JSRuntime*, JSValueConst data) {
                    delete take<NativeClassBase>(data);
                    JS_SetOpaque(data, nullptr);
                },
            .gc_mark =
                +[](JSRuntime* rt, JSValueConst data, JS_MarkFunc* func) {
                    take<NativeClassBase>(data)->gc_mark([rt, func](const ValueType& value) {
                        func(rt, reinterpret_cast<JSGCObjectHeader*>(JS_VALUE_GET_OBJ(value.value)));
                    });
                },
            .call = nullptr,
            .exotic = nullptr,
        };

        JS_NewClassID(JS_GetRuntime(env), &_classId);
        JS_NewClass(JS_GetRuntime(env), _classId, &classDef);
    }

    static JSValue make(JSContext* ctx, NativeClassBase* pointer)
    {
        auto obj = JS_NewObjectClass(ctx, _classId);
        JS_SetOpaque(obj, pointer);
        return obj;
    }

    template <typename Type>
    static Type* take(JSValueConst value)
    {
        auto ptr = static_cast<NativeClassBase*>(JS_GetOpaque(value, _classId));
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

inline ValueType ThrowTypeError(EnvType env, const std::string& err)
{
    JS_ThrowTypeError(env, "%s", err.c_str());
    return { env, JS_EXCEPTION };
}

// 必须要非常小心, 这里传入的回调不能持有Value, 内部的FuncHolder未实现gc_mark
inline FunctionType
    MakeFunction(EnvType env, const char* name, int argc, RawCallback func, std::function<void(NativeMarkerFunc)> run_marker = nullptr)
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
            auto data = NativePointerHolder::take<FuncHolder>(func_data[0]);
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

inline void BindGetter(
    ObjectType object,
    const char* prop,
    const char* name,
    RawCallback func,
    std::function<void(NativeMarkerFunc)> run_marker = nullptr)
{
    JS_DefinePropertyGetSet(
        object.Env(),
        object.peek(),
        JS_NewAtom(object.Env(), prop),
        maajs::MakeFunction(object.Env(), name, 0, func, run_marker).take(),
        JS_UNDEFINED,
        JS_PROP_ENUMERABLE);
}

inline ObjectType CallCtor(FunctionType ctor, std::vector<ValueType> args)
{
    auto rawArgs = QjsArray::__Trans(args, false);
    auto result = JS_CallConstructor(ctor.Env(), ctor.peek(), static_cast<int>(rawArgs.size()), rawArgs.data());
    return { ctor.Env(), result };
}

inline ValueType CallMember(ObjectType object, const char* prop, std::vector<ValueType> args)
{
    return object[prop].AsValue().As<FunctionType>().Call(object, args);
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
        if (val.IsFunction()) {
            return "function";
        }
        else {
            return std::format(
                "object[{}]",
                val.As<ObjectType>()["constructor"].AsValue().As<ObjectType>()["name"].AsValue().As<StringType>().Utf8Value());
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
    std::string descStr = val.ToString().Utf8Value();
    if (descStr.length() > 20) {
        descStr = descStr.substr(0, 17) + "...";
    }
    return std::format("{} [{}]", descStr, TypeOf(val));
}

inline void init(EnvType env)
{
    NativePointerHolder::init(env);
}

}
