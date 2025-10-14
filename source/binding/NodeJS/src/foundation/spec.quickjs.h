#pragma once

#include <functional>

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

struct QjsObjRef
{
    JSContext* context {};
    JSValue value;

    JSValueConst Value() const { return value; }

    void Unref() { JS_FreeValue(context, value); }
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

    JSValueConst operator[](size_t idx) const { return argv[idx]; }
};

using ValueType = JSValue;
using ConstValueType = JSValueConst;
using ObjectType = JSValue;
using ConstObjectType = JSValue;
using ObjectRefType = QjsObjRef;

using EnvType = QjsEnv;
using CallbackInfo = QjsCallbackInfo;
using RawCallback = std::function<ValueType(const CallbackInfo&)>;

struct QjsPointerBase
{
    virtual ~QjsPointerBase() = default;
};

struct QjsPointerHolder
{
    static JSClassID _classId;

    static void init(EnvType env)
    {
        static JSClassDef classDef = {
            .class_name = "__QjsPointerHolder",
            .finalizer =
                +[](JSRuntime*, JSValueConst data) { //
                    delete (QjsPointerBase*)JS_GetOpaque(data, _classId);
                },
        };

        JS_NewClassID(env.runtime, &_classId);
        JS_NewClass(env.runtime, _classId, &classDef);
    }

    static JSValue make(JSContext* ctx, QjsPointerBase* pointer)
    {
        auto obj = JS_NewObjectClass(ctx, _classId);
        JS_SetOpaque(obj, pointer);
        return obj;
    }

    template <typename Type>
    static Type* take(JSValueConst value)
    {
        auto ptr = static_cast<QjsPointerBase*>(JS_GetOpaque(value, _classId));
        return dynamic_cast<Type*>(ptr);
    }
};

inline void FreeValue(EnvType env, ValueType val)
{
    JS_FreeValue(env.context, val);
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
    return std::string(ptr, len);
}

inline ObjectRefType Persistent(EnvType env, ConstObjectType val)
{
    return { env.context, JS_DupValue(env.context, val) };
}

inline ValueType MakeFunction(EnvType env, const char* name, int argc, RawCallback func)
{
    struct FuncHolder : public QjsPointerBase
    {
        RawCallback func;

        FuncHolder(RawCallback func)
            : func(func)
        {
        }
    };

    auto data = QjsPointerHolder::make(env.context, new FuncHolder { func });
    auto result = JS_NewCFunctionData(
        env.context,
        +[](JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv, int, JSValueConst* func_data) -> JSValue {
            auto data = QjsPointerHolder::take<FuncHolder>(func_data[0]);
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

inline void BindValue(EnvType env, ConstObjectType object, const char* prop, ValueType value)
{
    JS_DefinePropertyValue(env.context, object, JS_NewAtom(env.context, prop), value, JS_PROP_ENUMERABLE);
}

inline void BindGetter(EnvType env, ConstObjectType object, const char* prop, const char* name, RawCallback func)
{
    JS_DefinePropertyGetSet(
        env.context,
        object,
        JS_NewAtom(env.context, prop),
        maajs::MakeFunction(env, name, 0, func),
        JS_UNDEFINED,
        JS_PROP_ENUMERABLE);
}

inline ValueType CallMember(EnvType env, ConstObjectType object, const char* prop, std::vector<ValueType> args)
{
    auto func = JS_GetPropertyStr(env.context, object, prop);
    auto result = JS_Call(env.context, func, object, args.size(), args.data());
    for (auto& arg : args) {
        JS_FreeValue(env.context, arg);
    }
    return result;
}

inline void init(EnvType env)
{
    QjsPointerHolder::init(env);
}

}
