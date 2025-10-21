#pragma once

#include <functional>

#include "spec.h"

namespace maajs
{

inline PromiseType
    PromiseThen(PromiseType pro, ObjectType capture, std::function<ValueType(const CallbackInfo& info, ObjectType capture)> action)
{
    auto capturePtr = std::make_shared<ObjectRefType>(PersistentObject(capture));
    return pro.Then(
        maajs::MakeFunction(
            pro.Env(),
            "",
            1,
            [capturePtr, action = std::move(action)](const CallbackInfo& info) { return action(info, capturePtr->Value()); },
            [capturePtr](auto marker) { marker(capturePtr->Value()); }));
}

inline PromiseType
    PromiseThen(PromiseType pro, FunctionType capture, std::function<ValueType(const CallbackInfo& info, FunctionType capture)> action)
{
    auto capturePtr = std::make_shared<FunctionRefType>(PersistentFunction(capture));
    return pro.Then(
        maajs::MakeFunction(
            pro.Env(),
            "",
            1,
            [capturePtr, action = std::move(action)](const CallbackInfo& info) { return action(info, capturePtr->Value()); },
            [capturePtr](auto marker) { marker(capturePtr->Value()); }));
}

inline std::string JsonStringify(EnvType env, ValueType val)
{
    auto json = env.Global()["JSON"].AsValue().As<ObjectType>();
    return CallMemberHelper<std::string>(json, "stringify", val);
}

inline ValueType JsonParse(EnvType env, std::string str)
{
    auto json = env.Global()["JSON"].AsValue().As<ObjectType>();
    auto result = CallMemberHelper<ValueType>(json, "parse", str);
    return result;
}

}
