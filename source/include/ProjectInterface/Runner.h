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
            void* callback_arg = nullptr,
            MaaResourceCallback resource_callback = nullptr,
            void* resource_callback_arg = nullptr,
            MaaNotificationCallback controller_callback = nullptr,
            void* controller_callback_arg = nullptr);
};

MAA_PROJECT_INTERFACE_NS_END
