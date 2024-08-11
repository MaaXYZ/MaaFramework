#pragma once

#include "Types.h"

#include "MaaFramework/MaaDef.h"

MAA_PROJECT_INTERFACE_NS_BEGIN

class Runner
{
public:
    static bool
        run(const RuntimeParam& param,
            MaaInstanceCallback callback = nullptr,
            MaaTransparentArg callback_arg = nullptr,
            MaaResourceCallback resource_callback = nullptr,
            MaaTransparentArg resource_callback_arg = nullptr,
            MaaNotificationCallback controller_callback = nullptr,
            MaaTransparentArg controller_callback_arg = nullptr);
};

MAA_PROJECT_INTERFACE_NS_END
