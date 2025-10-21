#pragma once

#include "spec.h"

namespace maajs
{

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
