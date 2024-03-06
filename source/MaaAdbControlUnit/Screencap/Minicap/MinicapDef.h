#pragma once

#include <cstdint>

#include "Conf/Conf.h"

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

    uint8_t version = 0;
    uint8_t size = 0;
    uint32_t pid = 0;
    uint32_t real_width = 0;
    uint32_t real_height = 0;
    uint32_t virt_width = 0;
    uint32_t virt_height = 0;
    uint8_t orientation = 0;
    uint8_t flags = 0;
};

#pragma pack(pop)

MAA_CTRL_UNIT_NS_END
