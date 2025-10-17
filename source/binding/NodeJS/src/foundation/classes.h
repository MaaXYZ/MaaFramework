#pragma once

#include <iostream>

#include "../foundation/spec.h"

namespace maajs
{

#ifdef MAA_JS_IMPL_IS_NODEJS

template <typename Inherit>
struct NativeClass
{
    static void init(EnvType env, ValueType& ctor, [[maybe_unused]] FunctionRefType* super = nullptr)
    {
        ctor = MakeFunction(env, Inherit::name, 0, [](const maajs::CallbackInfo& info) { //
            if (!info.IsConstructCall()) {
                return info.Env().Null();
            }

            auto impl = Inherit::ctor(info);
            if (!impl) {
                return info.Env().Null();
            }

            auto obj = info.This().As<Napi::Object>();

            napi_wrap(info.Env(), obj, impl, +[](napi_env, void* ptr, void*) { delete static_cast<Inherit*>(ptr); }, nullptr, nullptr);

            return obj.As<Napi::Value>();
        });

        ObjectType proto = ctor.As<Napi::Function>().Get("prototype").As<Napi::Object>();

        Inherit::init_proto(env, proto);
    }

    static Inherit* take(ValueType val)
    {
        void* result = nullptr;
        napi_unwrap(val.Env(), val, &result);
        auto rootImpl = static_cast<NativeClassBase*>(result);
        auto currImpl = rootImpl;
        while (currImpl) {
            auto impl = dynamic_cast<Inherit*>(currImpl);
            if (impl) {
                return impl;
            }
            currImpl = currImpl->child_instance;
        }
        return nullptr;
    }
};

#define MAA_JS_NATIVE_CLASS_STATIC_IMPL(Type)

#endif

#ifdef MAA_JS_IMPL_IS_QUICKJS

template <typename Inherit>
struct NativeClass
{
    static JSClassID classId;

    static void init(EnvType env, FunctionType& ctor, FunctionRefType* super = nullptr)
    {
        static JSClassDef classDef = {
            .class_name = Inherit::name,
            .finalizer = +[](JSRuntime*, JSValueConst data) { delete take(data); },
            .gc_mark =
                +[](JSRuntime* rt, JSValueConst data, JS_MarkFunc* func) {
                    auto rootImpl = static_cast<NativeClassBase*>(JS_GetOpaque(data, classId));
                    auto currImpl = rootImpl;
                    while (currImpl) {
                        currImpl->gc_mark([rt, func](const ValueType& value) {
                            func(rt, reinterpret_cast<JSGCObjectHeader*>(JS_VALUE_GET_OBJ(value.value)));
                        });
                        currImpl = currImpl->child_instance;
                    }
                },
        };

        JS_NewClassID(JS_GetRuntime(env), &classId);
        JS_NewClass(JS_GetRuntime(env), classId, &classDef);

        auto proto = QjsObject::New(env);

        std::shared_ptr<FunctionRefType> superPtr {};
        std::function<void(NativeMarkerFunc)> ctorMarker {};
        if (super) {
            superPtr = std::make_shared<FunctionRefType>(PersistentFunction(super->Value()));
            ctorMarker = [superPtr](auto marker) {
                marker(superPtr->Value());
            };
        }

        ctor = maajs::MakeFunction(
            env,
            Inherit::name,
            0,
            [superPtr](const maajs::CallbackInfo& info) -> ValueType {
                // TODO: 调用Inherit把参数提前拆分下
                // 即, 指导如何调用super
                JSValue obj;
                if (superPtr) {
                    std::vector<ValueType> args;
                    args.reserve(info.Length());
                    for (size_t i = 0; i < info.Length(); i++) {
                        args.push_back(info[i]);
                    }
                    obj = CallCtor(*superPtr, args).take();
                }
                else {
                    obj = JS_NewObjectClass(info.context, classId);
                }

                if (JS_IsException(obj)) {
                    return { info.Env(), obj };
                }

                auto impl = Inherit::ctor(info);
                if (!impl) {
                    return { info.Env(), JS_EXCEPTION };
                }

                auto rootImpl = static_cast<NativeClassBase*>(JS_GetOpaque(obj, classId));
                if (rootImpl) {
                    auto currImpl = rootImpl;
                    while (currImpl->child_instance) {
                        currImpl = currImpl->child_instance;
                    }
                    currImpl->child_instance = impl;
                    impl->super_instance = currImpl;
                }
                else {
                    JS_SetOpaque(obj, impl);
                }

                return { info.Env(), obj };
            },
            ctorMarker);

        JS_SetConstructorBit(env, ctor.peek(), true);
        JS_SetConstructor(env, ctor.peek(), proto.peek());

        if (super) {
            JS_SetPrototype(env, ctor.peek(), super->Value().peek());
        }

        Inherit::init_proto(env, proto);

        JS_SetClassProto(env, classId, proto.take());
    }

    static Inherit* take(ValueType val) { return take(val.peek()); }

    static Inherit* take(JSValueConst val)
    {
        auto rootImpl = static_cast<NativeClassBase*>(JS_GetOpaque(val, classId));
        auto currImpl = rootImpl;
        while (currImpl) {
            auto impl = dynamic_cast<Inherit*>(currImpl);
            if (impl) {
                return impl;
            }
            currImpl = currImpl->child_instance;
        }
        return nullptr;
    }
};

#define MAA_JS_NATIVE_CLASS_STATIC_IMPL(Type) \
    template <>                               \
    JSClassID maajs::NativeClass<Type>::classId {};

#endif

}
