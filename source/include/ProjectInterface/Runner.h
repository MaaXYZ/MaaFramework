#pragma once

#include "Types.h"

#include "MaaFramework/MaaDef.h"

MAA_PROJECT_INTERFACE_NS_BEGIN

class Runner
{
public:
    static bool run(const RuntimeParam& param, MaaNotificationCallback callback = nullptr, void* callback_arg = nullptr);
};

MAA_PROJECT_INTERFACE_NS_END
