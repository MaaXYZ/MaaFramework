#pragma once

#include <string>
#include <variant>
#include <vector>

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

struct QjsValue;
struct QjsObject;
struct QjsString;

struct QjsEnv
{
    JSContext* context {};

    operator JSContext*() const { return context; }

    QjsValue Null() const;
    QjsValue Undefined() const;
    QjsObject Global() const;

    JSRuntime* Runtime() const { return JS_GetRuntime(context); }
};

struct QjsValue
{
    JSContext* context {};
    JSValue value;

    QjsValue()
    {
        context = nullptr;
        value = JS_UNDEFINED;
    }

    explicit QjsValue(JSContext* context)
        : context(context)
        , value(JS_UNDEFINED)
    {
    }

    QjsValue(JSContext* context, JSValue value)
        : context(context)
        , value(value)
    {
    }

    QjsValue(const QjsValue& val)
        : context(val.context)
        , value(JS_DupValue(val.context, val.value))
    {
    }

    QjsValue(QjsValue&& val)
        : context(val.context)
        , value(val.value)
    {
        val.value = JS_UNDEFINED;
    }

    ~QjsValue()
    {
        if (context) {
            JS_FreeValue(context, value);
        }
    }

    QjsValue& operator=(const QjsValue& val)
    {
        if (&val == this) {
            return *this;
        }
        this->~QjsValue();
        context = val.context;
        value = JS_DupValue(val.context, val.value);
        return *this;
    }

    QjsValue& operator=(QjsValue&& val)
    {
        if (&val == this) {
            return *this;
        }
        this->~QjsValue();
        context = val.context;
        value = val.value;
        val.value = JS_UNDEFINED;
        return *this;
    }

    QjsEnv Env() const { return { context }; }

    template <typename Type>
    Type As() const
    {
        return Type { context, JS_DupValue(context, value) };
    }

    bool IsNull() const { return JS_IsNull(value); }

    bool IsUndefined() const { return JS_IsUndefined(value); }

    bool IsObject() const { return JS_IsObject(value); }

    bool IsBoolean() const { return JS_IsBool(value); }

    bool IsString() const { return JS_IsString(value); }

    bool IsNumber() const { return JS_IsNumber(value); }

    bool IsArray() const { return JS_IsArray(value); }

    bool IsFunction() const { return JS_IsFunction(Env(), value); }

    bool IsPromise() const { return JS_IsPromise(value); }

    bool IsArrayBuffer() const { return JS_IsArrayBuffer(value); }

    QjsString ToString() const;

    JSValue take()
    {
        JSValue ret = value;
        value = JS_UNDEFINED;
        return ret;
    }

    const JSValueConst& peek() const { return value; }
};

struct QjsObject : public QjsValue
{
    using QjsValue::QjsValue;

    static QjsObject New(QjsEnv env) { return { env, JS_NewObject(env) }; }

    template <typename Prop>
    struct QjsObjectPropRef
    {
        const QjsObject& obj;
        std::variant<std::string, uint32_t, int64_t> prop;

        void operator=(QjsValue val) const
        {
            if (const std::string* str_key = std::get_if<std::string>(&prop)) {
                JS_SetPropertyStr(obj.context, obj.value, str_key->c_str(), val.take());
            }
            else if (const uint32_t* ui32_key = std::get_if<uint32_t>(&prop)) {
                JS_SetPropertyUint32(obj.context, obj.value, *ui32_key, val.take());
            }
            else if (const int64_t* i64_key = std::get_if<int64_t>(&prop)) {
                JS_SetPropertyInt64(obj.context, obj.value, *i64_key, val.take());
            }
        }

        operator QjsValue() const
        {
            if (const std::string* str_key = std::get_if<std::string>(&prop)) {
                return {
                    obj.context,
                    JS_GetPropertyStr(obj.context, obj.value, str_key->c_str()),
                };
            }
            else if (const uint32_t* ui32_key = std::get_if<uint32_t>(&prop)) {
                return {
                    obj.context,
                    JS_GetPropertyUint32(obj.context, obj.value, *ui32_key),
                };
            }
            else if (const int64_t* i64_key = std::get_if<int64_t>(&prop)) {
                return {
                    obj.context,
                    JS_GetPropertyInt64(obj.context, obj.value, *i64_key),
                };
            }
            else {
                return obj.Env().Undefined();
            }
        }

        QjsValue AsValue() const { return *this; }
    };

    template <typename Prop>
    QjsObjectPropRef<Prop> operator[](Prop prop) const
    {
        return { *this, prop };
    }
};

struct QjsBoolean : public QjsValue
{
    using QjsValue::QjsValue;

    static QjsBoolean New(QjsEnv env, bool val) { return { env, JS_NewBool(env, val) }; }

    bool Value() const { return JS_VALUE_GET_BOOL(value); }
};

struct QjsNumber : public QjsValue
{
    using QjsValue::QjsValue;

    static QjsNumber New(QjsEnv env, int32_t val) { return { env, JS_NewInt32(env, val) }; }

    static QjsNumber New(QjsEnv env, uint32_t val) { return { env, JS_NewUint32(env, val) }; }

    static QjsNumber New(QjsEnv env, double val) { return { env, JS_NewFloat64(env, val) }; }

    int32_t Int32Value() const { return JS_VALUE_GET_INT(value); }

    uint32_t Uint32Value() const
    {
        uint32_t res = 0;
        JS_ToUint32(context, &res, value);
        return res;
    }
};

struct QjsString : public QjsValue
{
    using QjsValue::QjsValue;

    static QjsString New(QjsEnv env, std::string val) { return { env, JS_NewStringLen(env, val.c_str(), val.size()) }; }

    std::string Utf8Value() const
    {
        size_t len {};
        auto ptr = JS_ToCStringLen2(context, &len, value, false);
        auto ret = std::string(ptr, len);
        JS_FreeCString(context, ptr);
        return ret;
    }
};

struct QjsArray : public QjsObject
{
    using QjsObject::QjsObject;

    static std::vector<JSValue> __Trans(std::vector<QjsValue> vals, bool take)
    {
        std::vector<JSValue> rawVals;
        rawVals.reserve(vals.size());
        for (auto& val : vals) {
            rawVals.push_back(take ? val.take() : val.peek());
        }
        return rawVals;
    }

    static QjsArray Make(QjsEnv env, std::vector<QjsValue> vals)
    {
        auto rawVals = __Trans(vals, true);
        return {
            env,
            JS_NewArrayFrom(env, static_cast<int>(rawVals.size()), rawVals.data()),
        };
    }

    size_t Length() const
    {
        auto lenVar = static_cast<const QjsObject*>(this)->operator[]("length").AsValue();
        // TODO: 这里强行缩减到了int32, 是不是还是换个办法
        return static_cast<uint64_t>(lenVar.As<QjsNumber>().Int32Value());
    }

    QjsValue operator[](size_t idx) const
    {
        return {
            context,
            JS_GetPropertyInt64(context, value, idx),
        };
    }
};

struct QjsFunction : public QjsObject
{
    using QjsObject::QjsObject;

    QjsValue Call(std::vector<QjsValue> args) const { return Call(Env().Undefined(), args); }

    QjsValue Call(QjsValue recv, std::vector<QjsValue> args) const
    {
        auto rawArgs = QjsArray::__Trans(args, false);
        auto result = JS_Call(Env(), peek(), recv.peek(), static_cast<int>(rawArgs.size()), rawArgs.data());
        return { Env(), result };
    }
};

struct QjsPromise : public QjsObject
{
    using QjsObject::QjsObject;

    QjsPromise Then(QjsFunction onFulfilled) const
    {
        return this->operator[]("then")
            .AsValue()
            .As<QjsFunction>()
            .Call(
                *this,
                {
                    onFulfilled,
                })
            .As<QjsPromise>();
    }

    QjsPromise Catch(QjsFunction onRejected) const
    {
        return this->operator[]("catch")
            .AsValue()
            .As<QjsFunction>()
            .Call(
                *this,
                {
                    onRejected,
                })
            .As<QjsPromise>();
    }
};

struct QjsArrayBuffer : public QjsObject
{
    using QjsObject::QjsObject;

    static QjsArrayBuffer New(QjsEnv env, size_t len) { return { env, JS_NewArrayBufferCopy(env, nullptr, len) }; }

    void* Data() const
    {
        size_t size {};
        return JS_GetArrayBuffer(Env(), &size, peek());
    }

    size_t ByteLength() const
    {
        size_t size {};
        JS_GetArrayBuffer(Env(), &size, peek());
        return size;
    }
};

struct QjsCallbackInfo

{
    JSContext* context {};
    JSValueConst thisObject;
    int argc {};
    JSValueConst* argv;

    QjsEnv Env() const { return { context }; }

    QjsValue This() const { return { context, JS_DupValue(context, thisObject) }; }

    size_t Length() const { return static_cast<size_t>(argc); }

    QjsValue operator[](size_t idx) const { return { context, JS_DupValue(context, argv[idx]) }; }
};

template <typename Target>
struct QjsRef : public Target
{
    const Target& Value() const { return *this; }
};

template <typename Target>
struct QjsWeakRef : public QjsObject
{
    using QjsObject::QjsObject;

    static QjsWeakRef<Target> Make(Target target)
    {
        QjsEnv env = target.Env();
        auto weakRefCtor = env.Global()["WeakRef"].AsValue().As<QjsFunction>();
        JSValueConst args = target.peek();
        auto ref = JS_CallConstructor(env, weakRefCtor.peek(), 1, &args);
        return { env, ref };
    }

    Target Value()
    {
        auto derefFunc = this->operator[]("deref").AsValue().template As<QjsFunction>();
        auto target = JS_Call(Env(), derefFunc.peek(), peek(), 0, nullptr);
        QjsValue targetVal = { Env(), target };
        // 这里迁就Napi中的行为，weakref失效了返回null
        if (targetVal.IsUndefined()) {
            targetVal = Env().Null();
        }
        return targetVal.As<Target>();
    }
};

inline QjsValue QjsEnv::Null() const

{
    return { context, JS_NULL };
};

inline QjsValue QjsEnv::Undefined() const
{
    return { context, JS_UNDEFINED };
}

inline QjsObject QjsEnv::Global() const
{
    return { context, JS_GetGlobalObject(context) };
}

inline QjsString QjsValue::ToString() const
{
    return { context, JS_ToString(context, peek()) };
}

}
