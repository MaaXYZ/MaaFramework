#pragma once

#include <iostream>

#include "../instric.h"
#include "../tools.h"
#include "../types.h"

namespace maajs
{

template <typename Inherit>
struct NativeClass
{
    template <typename Super = void>
    static void init(EnvType env, FunctionType& ctor, FunctionRefType* superCtor = nullptr)
    {
        ctor = MakeFunction(env, Inherit::name, 0, [](const CallbackInfo& info) { //
            if (!info.IsConstructCall()) {
                return info.Env().Null();
            }

            try {
                auto impl = Inherit::ctor(info);
                if (!impl) {
                    return info.Env().Null();
                }

                impl->env = info.Env();

                auto obj = info.This().As<Napi::Object>();

                napi_wrap(info.Env(), obj, impl, +[](napi_env, void* ptr, void*) { delete static_cast<Inherit*>(ptr); }, nullptr, nullptr);

                impl->init_bind(obj);

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

        bind_to_string(proto);

        Inherit::init_proto(proto, ctor);
    }

    static Inherit* take(ValueType val)
    {
        void* result = nullptr;
        napi_unwrap(val.Env(), val, &result);
        return dynamic_cast<Inherit*>(static_cast<NativeClassBase*>(result));
    }

    static void bind_to_string(ObjectType proto);
};

}
