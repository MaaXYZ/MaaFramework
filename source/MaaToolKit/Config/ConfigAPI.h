#pragma once

#include <string_view>

#include "Conf/Conf.h"
#include "MaaToolKit/MaaToolKitDef.h"

struct MaaToolKitConfigMgrAPI
{
    virtual ~MaaToolKitConfigMgrAPI() = default;

    virtual bool init() = 0;
    virtual bool uninit() = 0;
};
