#pragma once

#include <format>
#include <functional>
#include <iostream>
#include <string>
#include <typeinfo>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4244)
#endif
#include <quickjs.h> // IWYU pragma: export
#ifdef _MSC_VER
#pragma warning(pop)
#endif

namespace maajs
{

struct QjsEnv
{
    JSRuntime* runtime {};
    JSContext* context {};
};

struct QjsRef
{
    JSContext* context {};
    JSValue value;

    bool auto_unref {};

    QjsRef() = default;
    QjsRef(JSContext* ctx, JSValue val, bool unref)
        : context(ctx)
        , value(val)
        , auto_unref(unref) {};
    QjsRef(const QjsRef&) = delete;

    QjsRef(QjsRef&& ref)
    {
        context = ref.context;
        value = ref.value;
        auto_unref = ref.auto_unref;
        ref.auto_unref = false;
    }

    ~QjsRef()
    {
        if (auto_unref) {
            Unref();
        }
    }

    QjsRef& operator=(const QjsRef&) = delete;

    QjsRef& operator=(QjsRef&& ref)
    {
        if (&ref == this) {
            return *this;
        }
        if (auto_unref) {
            Unref();
        }
        context = ref.context;
        value = ref.value;
        auto_unref = ref.auto_unref;
        ref.auto_unref = false;
        return *this;
    }

    JSValueConst Value() const { return value; }

    void Unref()
    {
        if (context) {
            JS_FreeValue(context, value);
        }
    }
};

struct QjsPromise
{
    JSContext* context {};
    JSValue value;

    operator JSValue() const { return value; }
};

struct QjsCallbackInfo
{
    JSContext* context {};
    JSValueConst thisObject;
    int argc {};
    JSValueConst* argv;

    QjsEnv Env() const
    {
        return {
            JS_GetRuntime(context),
            context,
        };
    }

    JSValueConst This() const { return thisObject; }

    size_t Length() const { return static_cast<size_t>(argc); }

    JSValueConst operator[](size_t idx) const { return argv[idx]; }
};

using ValueType = JSValue;
using ConstValueType = JSValueConst;
using ObjectType = JSValue;
using ConstObjectType = JSValue;
using ObjectRefType = QjsRef;
using FunctionRefType = QjsRef;
using PromiseType = QjsPromise;

using EnvType = QjsEnv;
using CallbackInfo = QjsCallbackInfo;
using RawCallback = std::function<ValueType(const CallbackInfo&)>;

using NativeMarkerFunc = std::function<void(maajs::ConstValueType)>;

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
                    take<NativeClassBase>(data)->gc_mark(
                        [rt, func](ConstValueType value) { func(rt, reinterpret_cast<JSGCObjectHeader*>(JS_VALUE_GET_OBJ(value))); });
                },
        };

        JS_NewClassID(env.runtime, &_classId);
        JS_NewClass(env.runtime, _classId, &classDef);
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

inline ValueType DupValue(EnvType env, ConstValueType val)
{
    return JS_DupValue(env.context, val);
}

inline void FreeValue(EnvType env, ValueType val)
{
    JS_FreeValue(env.context, val);
}

inline ValueType ObjectToValue(ObjectType val)
{
    return val;
}

inline ObjectType ValueToObject(ValueType val)
{
    return val;
}

inline ValueType MakeNull(EnvType)
{
    return JS_NULL;
}

inline bool IsNull(ConstValueType val)
{
    return JS_IsNull(val);
}

inline ValueType MakeUndefined(EnvType)
{
    return JS_UNDEFINED;
}

inline bool IsUndefined(ConstValueType val)
{
    return JS_IsUndefined(val);
}

inline ValueType MakeNumber(EnvType env, int32_t val)
{
    return JS_NewInt32(env.context, val);
}

inline ValueType MakeNumber(EnvType env, double val)
{
    return JS_NewFloat64(env.context, val);
}

inline bool IsNumber(ConstValueType val)
{
    return JS_IsNumber(val);
}

inline int32_t GetNumberI32(ConstValueType val)
{
    return JS_VALUE_GET_INT(val);
}

inline ObjectType MakeObject(EnvType env)
{
    return JS_NewObject(env.context);
}

inline ValueType MakeBool(EnvType env, bool val)
{
    return JS_NewBool(env.context, val);
}

inline bool IsBool(ConstValueType val)
{
    return JS_IsBool(val);
}

inline bool GetBool(ConstValueType val)
{
    return JS_VALUE_GET_BOOL(val);
}

inline ValueType MakeString(EnvType env, std::string value)
{
    return JS_NewStringLen(env.context, value.c_str(), value.size());
}

inline bool IsString(ConstValueType val)
{
    return JS_IsString(val);
}

inline std::string GetString(EnvType env, ConstValueType val)
{
    size_t len {};
    auto ptr = JS_ToCStringLen2(env.context, &len, val, false);
    auto ret = std::string(ptr, len);
    JS_FreeCString(env.context, ptr);
    return ret;
}

inline ValueType MakeArray(EnvType env, std::vector<ValueType> vals)
{
    return JS_NewArrayFrom(env.context, static_cast<int>(vals.size()), vals.data());
}

inline bool IsArray(ConstValueType val)
{
    return JS_IsArray(val);
}

inline std::vector<ConstValueType> GetArray(EnvType env, ConstValueType val)
{
    auto lenVal = JS_GetPropertyStr(env.context, val, "length");
    uint32_t len = static_cast<uint32_t>(JS_VALUE_GET_INT(lenVal));
    std::vector<ConstValueType> result;
    result.reserve(len);
    for (uint32_t i = 0; i < len; i++) {
        result.push_back(JS_GetPropertyUint32(env.context, val, i));
    }
    return result;
}

inline ObjectRefType PersistentObject(EnvType env, ConstObjectType val, bool auto_unref = false)
{
    return { env.context, JS_DupValue(env.context, val), auto_unref };
}

inline FunctionRefType PersistentFunction(EnvType env, ConstObjectType val, bool auto_unref = false)
{
    return { env.context, JS_DupValue(env.context, val), auto_unref };
}

// 必须要非常小心, 这里传入的回调不能持有Value, 内部的FuncHolder未实现gc_mark
inline ValueType
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
        env.context,
        new FuncHolder {
            func,
            run_marker,
        });
    auto result = JS_NewCFunctionData(
        env.context,
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
            return data->func(info);
        },
        argc,
        0,
        1,
        &data);
    JS_FreeValue(env.context, data);

    JS_DefinePropertyValue(env.context, result, 55 /* JS_ATOM_name */, JS_NewString(env.context, name), JS_PROP_CONFIGURABLE);

    return result;
}

inline std::tuple<PromiseType, std::shared_ptr<FunctionRefType>, std::shared_ptr<FunctionRefType>> MakePromise(EnvType env)
{
    JSValue funcs[2];
    auto retPro = JS_NewPromiseCapability(env.context, funcs);

    auto resolvePtr = std::make_shared<maajs::FunctionRefType>(maajs::PersistentFunction(env, funcs[0], true));
    maajs::FreeValue(env, funcs[0]);

    auto rejectPtr = std::make_shared<maajs::FunctionRefType>(maajs::PersistentFunction(env, funcs[1], true));
    maajs::FreeValue(env, funcs[1]);

    return { { env.context, retPro }, resolvePtr, rejectPtr };
}

inline ValueType GetProp(EnvType env, ConstObjectType object, const char* prop)
{
    return JS_GetPropertyStr(env.context, object, prop);
}

inline void BindValue(EnvType env, ConstObjectType object, const char* prop, ValueType value)
{
    JS_DefinePropertyValue(env.context, object, JS_NewAtom(env.context, prop), value, JS_PROP_ENUMERABLE);
}

inline void BindGetter(
    EnvType env,
    ConstObjectType object,
    const char* prop,
    const char* name,
    RawCallback func,
    std::function<void(NativeMarkerFunc)> run_marker = nullptr)
{
    JS_DefinePropertyGetSet(
        env.context,
        object,
        JS_NewAtom(env.context, prop),
        maajs::MakeFunction(env, name, 0, func, run_marker),
        JS_UNDEFINED,
        JS_PROP_ENUMERABLE);
}

inline ValueType CallCtor(EnvType env, const FunctionRefType& ctor, std::vector<ValueType> args)
{
    auto result = JS_CallConstructor(env.context, ctor.Value(), static_cast<int>(args.size()), args.data());
    for (auto& arg : args) {
        JS_FreeValue(env.context, arg);
    }
    return result;
}

inline ValueType CallFunc(EnvType env, ConstValueType func, std::vector<ValueType> args)
{
    auto result = JS_Call(env.context, func, JS_UNDEFINED, static_cast<int>(args.size()), args.data());
    for (auto& arg : args) {
        JS_FreeValue(env.context, arg);
    }
    return result;
}

inline ValueType CallMember(EnvType env, ConstObjectType object, const char* prop, std::vector<ValueType> args)
{
    auto func = JS_GetPropertyStr(env.context, object, prop);
    auto result = JS_Call(env.context, func, object, static_cast<int>(args.size()), args.data());
    JS_FreeValue(env.context, func);
    for (auto& arg : args) {
        JS_FreeValue(env.context, arg);
    }
    return result;
}

inline std::string_view TypeOf(EnvType env, ConstValueType val)
{
    switch (val.tag) {
    case JS_TAG_NULL:
        return "null";
    }
    switch (val.tag) {
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
        if (JS_IsFunction(env.context, val)) {
            return "function";
        }
        else {
            return "object";
        }
    case JS_TAG_BIG_INT:
    case JS_TAG_SHORT_BIG_INT:
        return "bigint";
    case JS_TAG_EXCEPTION:
        return "exception";
    }
    return "unknown";
}

inline std::string DumpValue(EnvType env, ConstValueType val)
{
    auto desc = JS_ToString(env.context, val);
    std::string descStr = GetString(env, desc);
    JS_FreeValue(env.context, desc);
    if (descStr.length() > 20) {
        descStr = descStr.substr(0, 17) + "...";
    }
    return std::format("{} [{}]", descStr, TypeOf(env, val));
}

inline ObjectType GetGlobal(EnvType env)
{
    return JS_GetGlobalObject(env.context);
}

inline void init(EnvType env)
{
    NativePointerHolder::init(env);
}

}
