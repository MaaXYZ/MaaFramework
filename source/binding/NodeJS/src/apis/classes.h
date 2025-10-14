#pragma once

#include "../foundation/spec.h"

struct JobImpl;
struct ResourceImpl;

namespace maajs
{

template <size_t N>
struct StringHolder
{
    char data[N];

    constexpr StringHolder(const char (&str)[N]) { std::copy(str, str + N, data); }
};

#ifdef MAA_JS_IMPL_IS_NODEJS

template <typename Inherit>
struct NativeClass
{
    static void init(EnvType env, ValueType& ctor)
    {
        ctor = MakeFunction(env, Inherit::name, 0, [](const maajs::CallbackInfo& info) { //
            if (!info.IsConstructCall()) {
                return MakeNull(info.Env());
            }

            auto impl = Inherit::ctor(info);
            if (!impl) {
                return MakeNull(info.Env());
            }

            auto obj = info.This().As<Napi::Object>();

            napi_wrap(info.Env(), obj, impl, +[](napi_env, void* ptr, void*) { delete static_cast<Inherit*>(ptr); }, nullptr, nullptr);

            return obj.As<Napi::Value>();
        });

        ObjectType proto = ctor.As<Napi::Function>().Get("prototype").As<Napi::Object>();
        Inherit::init_proto(env, proto);
    }

    static Inherit* take(ConstValueType val)
    {
        void* result = nullptr;
        napi_unwrap(val.Env(), val, &result);
        return static_cast<Inherit*>(result);
    }
};

#define MAA_JS_NATIVE_CLASS_STATIC_IMPL(Type)

#endif

#ifdef MAA_JS_IMPL_IS_QUICKJS

template <typename Inherit>
struct NativeClass
{
    static JSClassID classId;

    static void init(EnvType env, ValueType& ctor)
    {
        static JSClassDef classDef = {
            .class_name = Inherit::name,
            .finalizer = +[](JSRuntime*, JSValueConst data) { delete (Inherit*)JS_GetOpaque(data, classId); },
        };

        JS_NewClassID(env.runtime, &classId);
        JS_NewClass(env.runtime, classId, &classDef);

        auto proto = maajs::MakeObject(env);

        ctor = maajs::MakeFunction(env, Inherit::name, 0, [](const maajs::CallbackInfo& info) { //
            auto obj = JS_NewObjectClass(info.context, classId);
            if (JS_IsException(obj)) {
                return obj;
            }

            auto impl = Inherit::ctor(info);
            if (!impl) {
                return JS_EXCEPTION;
            }

            JS_SetOpaque(obj, impl);
            return obj;
        });

        JS_SetConstructorBit(env.context, ctor, true);
        JS_SetConstructor(env.context, ctor, proto);

        Inherit::init_proto(env, proto);

        JS_SetClassProto(env.context, classId, proto);
    }

    static Inherit* take(ConstValueType val) { return static_cast<Inherit*>(JS_GetOpaque(val, classId)); }
};

#define MAA_JS_NATIVE_CLASS_STATIC_IMPL(Type) \
    template <>                               \
    JSClassID maajs::Type::classId {};

#endif

using JobNative = NativeClass<JobImpl>;
using ResourceNative = NativeClass<ResourceImpl>;

}
