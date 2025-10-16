#pragma once

#include "spec.h"

namespace maajs
{
inline std::string JsonStringify(EnvType env, ConstValueType val)
{
    auto global = GetGlobal(env);
    auto json = GetProp(env, global, "JSON");
    FreeValue(env, global);
    auto result = CallMember(
        env,
        ValueToObject(json),
        "stringify",
        {
            DupValue(env, val),
        });
    FreeValue(env, json);
    auto str = GetString(env, result);
    FreeValue(env, result);
    return str;
}

}
