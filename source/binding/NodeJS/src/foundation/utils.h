#pragma once

#include "spec.h"

namespace maajs
{
inline std::string JsonStringify(EnvType env, ValueType val)
{
    auto global = env.Global();
    auto json = global["JSON"].AsValue().As<ObjectType>();
    auto result = CallMember(json, "stringify", { val });
    return result.As<StringType>().Utf8Value();
}

}
