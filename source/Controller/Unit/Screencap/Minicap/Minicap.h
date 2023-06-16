#pragma once

#include "Common/MaaConf.h"

#include <cstdint>

MAA_CTRL_UNIT_NS_BEGIN

#pragma pack(push, 1)

struct MinicapHeader
{
    enum
    {
        DUMB = 1 << 0,
        ALWAYS_UPRIGHT = 1 << 1,
        TEAR = 1 << 2
    };

    uint8_t version; // 1
    uint8_t size;
    // 下面几个其实应该是uint32_t, 但是图方便
    int32_t pid;
    int32_t realWidth;
    int32_t realHeight;
    int32_t virtWidth;
    int32_t virtHeight;
    uint8_t orientation;
    uint8_t flags;
};

#pragma pack(pop)

MAA_CTRL_UNIT_NS_END
