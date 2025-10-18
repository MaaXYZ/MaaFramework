#pragma once

#include <cassert>
#include <iostream>

#include "error.h"
#include "spec.h"

namespace maajs
{

#ifdef MAA_JS_IMPL_IS_NODEJS

template <typename Inherit>
struct NativeClass
{
    template <typename Super = void>
    static void init(EnvType env, FunctionType& ctor, FunctionRefType* superCtor = nullptr)
    {
        ctor = MakeFunction(env, Inherit::name, 0, [](const maajs::CallbackInfo& info) { //
            if (!info.IsConstructCall()) {
                return info.Env().Null();
            }

            try {
                auto impl = Inherit::ctor(info);
                if (!impl) {
                    return info.Env().Null();
                }

                auto obj = info.This().As<Napi::Object>();

                napi_wrap(info.Env(), obj, impl, +[](napi_env, void* ptr, void*) { delete static_cast<Inherit*>(ptr); }, nullptr, nullptr);

                return obj.As<Napi::Value>();
            }
            catch (const MaaError& exc) {
                std::string what = std::format("maa.{}.ctor: {}", Inherit::name, exc.what());
                return ThrowTypeError(info.Env(), what);
            }

        });

        ObjectType proto = ctor.Get("prototype").As<Napi::Object>();

        if constexpr (!std::is_same_v<Super, void>) {
            auto superProto = superCtor->Value().Get("prototype");
            auto objectObj = env.Global()["Object"].AsValue().As<Napi::Object>();
            objectObj["setPrototypeOf"].AsValue().As<Napi::Function>().Call(objectObj, { proto, superProto });
        }

        Inherit::init_proto(env, proto, ctor);
    }

    static Inherit* take(ValueType val)
    {
        void* result = nullptr;
        napi_unwrap(val.Env(), val, &result);
        return dynamic_cast<Inherit*>(static_cast<NativeClassBase*>(result));
    }
};

#define MAA_JS_NATIVE_CLASS_STATIC_FORWARD(Type)
#define MAA_JS_NATIVE_CLASS_STATIC_IMPL(Type)

#endif

#ifdef MAA_JS_IMPL_IS_QUICKJS

struct NativeClassIDChain
{
    JSClassID classId {};
    std::vector<JSClassID> possibleIds {};
    NativeClassIDChain* superId {};
};

template <typename Inherit>
struct NativeClass
{
    static NativeClassIDChain classId;

    template <typename Super = void>
    static void init(EnvType env, FunctionType& ctor, FunctionRefType* = nullptr)
    {
        static JSClassDef classDef = {
            .class_name = Inherit::name,
            .finalizer = +[](JSRuntime*, JSValueConst data) { delete take(data); },
            .gc_mark =
                +[](JSRuntime* rt, JSValueConst data, JS_MarkFunc* func) {
                    take(data)->gc_mark([rt, func](const ValueType& value) {
                        func(rt, reinterpret_cast<JSGCObjectHeader*>(JS_VALUE_GET_OBJ(value.value)));
                    });
                },
            .call = nullptr,
            .exotic = nullptr,
        };

        JS_NewClassID(JS_GetRuntime(env), &classId.classId);
        JS_NewClass(JS_GetRuntime(env), classId.classId, &classDef);

        classId.possibleIds.push_back(classId.classId);

        std::cerr << Inherit::name << ": " << classId.classId << std::endl;

        auto proto = QjsObject::New(env);

        ctor = maajs::MakeFunction(env, Inherit::name, 0, [](const maajs::CallbackInfo& info) -> ValueType {
            auto obj = JS_NewObjectClass(info.context, classId.classId);

            if (JS_IsException(obj)) {
                return { info.Env(), obj };
            }

            try {
                auto impl = Inherit::ctor(info);
                if (!impl) {
                    return { info.Env(), JS_EXCEPTION };
                }

                JS_SetOpaque(obj, impl);

                return { info.Env(), obj };
            }
            catch (const MaaError& exc) {
                std::string what = std::format("maa.{}.ctor: {}", Inherit::name, exc.what());
                return ThrowTypeError(info.Env(), what);
            }
        });

        JS_SetConstructorBit(env, ctor.peek(), true);
        JS_SetConstructor(env, ctor.peek(), proto.peek());

        if constexpr (!std::is_same_v<Super, void>) {
            std::cerr << "Link " << classId.classId << " to " << NativeClass<Super>::classId.classId << std::endl;
            auto super = &NativeClass<Super>::classId;
            classId.superId = super;
            while (super) {
                super->possibleIds.push_back(classId.classId);
                super = super->superId;
            }

            auto superProto = JS_GetClassProto(env, classId.superId->classId);
            JS_SetPrototype(env, proto.peek(), superProto);
            JS_FreeValue(env, superProto);
        }

        Inherit::init_proto(env, proto, ctor);

        JS_SetClassProto(env, classId.classId, proto.take());
    }

    static Inherit* take(ValueType val) { return take(val.peek()); }

    static Inherit* take(JSValueConst val)
    {
        for (auto id : classId.possibleIds) {
            auto impl = static_cast<NativeClassBase*>(JS_GetOpaque(val, id));
            if (impl) {
                return dynamic_cast<Inherit*>(impl);
            }
        }
        return nullptr;
    }
};

#define MAA_JS_NATIVE_CLASS_STATIC_FORWARD(Type) \
    template <>                                  \
    maajs::NativeClassIDChain maajs::NativeClass<Type>::classId;

#define MAA_JS_NATIVE_CLASS_STATIC_IMPL(Type) \
    template <>                               \
    maajs::NativeClassIDChain maajs::NativeClass<Type>::classId {};

#endif

}
