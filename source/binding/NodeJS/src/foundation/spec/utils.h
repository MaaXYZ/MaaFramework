#pragma once

#include <format>
#include <utility>

#include "wrapper.h"

#define MAA_WRAP_FUNC(func) maajs::WrapFunctionHelper<decltype(&func), &func, #func>::make()
#define MAA_WRAP_FUNC_VALUE(env, func) maajs::WrapFunctionHelper<decltype(&func), (&func), #func>::makeValue(env)

#define MAA_BIND_FUNC(object, prop, func) maajs::BindValue(object, prop, MAA_WRAP_FUNC_VALUE(object.Env(), func))
#define MAA_BIND_GETTER(object, prop, func) \
    maajs::BindGetterSetter(object, prop, #func, "phony setter for " prop, MAA_WRAP_FUNC(func), nullptr)
#define MAA_BIND_SETTER(object, prop, func) \
    maajs::BindGetterSetter(object, prop, "phony getter for " prop, #func, nullptr, MAA_WRAP_FUNC(func))
#define MAA_BIND_GETTER_SETTER(object, prop, getter, setter) \
    maajs::BindGetterSetter(object, prop, #getter, #setter, MAA_WRAP_FUNC(getter), MAA_WRAP_FUNC(setter))

namespace maajs
{

template <typename... Args>
inline ObjectType CallCtorHelper(const FunctionRefType& ctor, Args&&... args)
{
    auto params = WrapArgs(ctor.Env(), std::forward<Args>(args)...);

    auto val = CallCtor(ctor, params);

    return val;
}

template <typename Ret, typename... Args>
inline Ret CallFuncHelper(FunctionType func, Args&&... args)
{
    auto params = WrapArgs(func.Env(), std::forward<Args>(args)...);

    auto val = func.Call(params);

    if constexpr (!std::is_same_v<Ret, void>) {
        return JSConvert<Ret>::from_value(val);
    }
}

template <typename Ret, typename... Args>
inline Ret CallMemberHelper(ObjectType object, const char* prop, Args&&... args)
{
    auto params = WrapArgs(object.Env(), std::forward<Args>(args)...);

    auto val = object[prop].AsValue().As<FunctionType>().Call(object, params);

    if constexpr (!std::is_same_v<Ret, void>) {
        return JSConvert<Ret>::from_value(val);
    }
}

// post impl

template <typename Inherit>
inline void NativeClass<Inherit>::bind_to_string(ObjectType proto)
{
    auto func = maajs::MakeFunction(
        proto.Env(),
        std::format("{}.toString", Inherit::name).c_str(),
        0,
        +[](const CallbackInfo& info) {
            auto impl = NativeClass<Inherit>::take(info.This());
            if (!impl) {
                return StringType::New(info.Env(), std::format("{} {{}}", TypeOf(info.This())));
            }
            else {
                return StringType::New(info.Env(), std::format("{} {{{}}}", Inherit::name, impl->to_string()));
            }
        });
    maajs::BindValue(proto, "toString", func);
#ifdef MAA_JS_IMPL_IS_NODEJS
    // 虽然是接受三个参数, 但是无所谓了
    proto.DefineProperty(Napi::PropertyDescriptor::Value(Napi::Symbol::For(proto.Env(), "nodejs.util.inspect.custom"), func));
#endif
}

}
