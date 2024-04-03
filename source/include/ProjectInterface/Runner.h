#pragma once

#include "Types.h"

#include "MaaFramework/MaaDef.h"

MAA_PROJECT_INTERFACE_NS_BEGIN

class Runner
{
public:
    static bool
        run(const MAA_PROJECT_INTERFACE_NS::RuntimeParam& param, //
            MaaInstanceCallback callback = nullptr,
            MaaCallbackTransparentArg callback_arg = nullptr,
            MaaResourceCallback resource_callback = nullptr,
            MaaCallbackTransparentArg resource_callback_arg = nullptr,
            MaaControllerCallback controller_callback = nullptr,
            MaaCallbackTransparentArg controller_callback_arg = nullptr);
};

MAA_PROJECT_INTERFACE_NS_END
