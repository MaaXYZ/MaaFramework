#pragma once

#include <MaaFramework/MaaAPI.h>

#include "../foundation/spec.h"

namespace maajs
{

template <>
struct JSConvert<MaaRect>
{
    static std::string name() { return "object<Rect>"; };

    static MaaRect from_value(ValueType val)
    {
        if (val.IsObject()) {
            auto obj = val.As<ObjectType>();
            return {
                .x = JSConvert<int32_t>::from_value(obj["x"]),
                .y = JSConvert<int32_t>::from_value(obj["y"]),
                .width = JSConvert<int32_t>::from_value(obj["width"]),
                .height = JSConvert<int32_t>::from_value(obj["height"]),
            };
        }
        throw MaaError { std::format("expect {}, got {}", name(), DumpValue(val)) };
    }

    static ValueType to_value(EnvType env, const MaaRect& val)
    {
        auto obj = ObjectType::New(env);
        obj["x"] = NumberType::New(env, val.x);
        obj["y"] = NumberType::New(env, val.y);
        obj["width"] = NumberType::New(env, val.width);
        obj["height"] = NumberType::New(env, val.height);
        return obj;
    }
};

}
