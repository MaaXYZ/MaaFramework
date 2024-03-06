#pragma once

#include <string_view>

#include "Conf/Conf.h"
#include "MaaToolkit/MaaToolkitDef.h"

struct MaaToolkitConfigMgrAPI
{
    virtual ~MaaToolkitConfigMgrAPI() = default;

    virtual bool init() = 0;
    virtual bool uninit() = 0;
};
