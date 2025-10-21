#pragma once

#include <iostream>

#include "../instric.h"
#include "../tools.h"
#include "../types.h"

namespace maajs
{

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

        ctor = MakeFunction(env, Inherit::name, 0, [](const CallbackInfo& info) -> ValueType {
            auto obj = JS_NewObjectClass(info.context, classId.classId);

            if (JS_IsException(obj)) {
                return { info.Env(), obj };
            }

            try {
                auto impl = Inherit::ctor(info);
                if (!impl) {
                    return { info.Env(), JS_EXCEPTION };
                }

                impl->env = info.Env();

                JS_SetOpaque(obj, impl);

                auto self = QjsObject { info.Env(), obj };
                impl->init_bind(self);

                return self;
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

        bind_to_string(proto);

        Inherit::init_proto(proto, ctor);

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

    static void bind_to_string(ObjectType proto);
};

}

#define MAA_JS_NATIVE_CLASS_STATIC_FORWARD(Type) \
    template <>                                  \
    maajs::NativeClassIDChain maajs::NativeClass<Type>::classId;

#define MAA_JS_NATIVE_CLASS_STATIC_IMPL(Type) \
    template <>                               \
    maajs::NativeClassIDChain maajs::NativeClass<Type>::classId {};

