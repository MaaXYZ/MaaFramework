#pragma once

#include <MaaFramework/MaaAPI.h>

#include "../foundation/spec.h"

namespace maajs
{

template <>
struct JSConvert<MaaRect>
{
    static std::string name() { return "array<Rect>"; };

    static MaaRect from_value(ValueType val)
    {
        auto arr = JSConvert<std::array<int32_t, 4>>::from_value(val);
        return {
            .x = arr[0],
            .y = arr[1],
            .width = arr[2],
            .height = arr[3],
        };
    }

    static ValueType to_value(EnvType env, const MaaRect& val)
    {
        return JSConvert<std::array<int32_t, 4>>::to_value(
            env,
            {
                val.x,
                val.y,
                val.width,
                val.height,
            });
    }
};

}
