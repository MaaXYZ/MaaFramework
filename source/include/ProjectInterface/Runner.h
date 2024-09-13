#pragma once

#include "Types.h"

#include "MaaFramework/MaaDef.h"

MAA_PROJECT_INTERFACE_NS_BEGIN

class Runner
{
public:
    static bool
        run(const RuntimeParam& param,
            MaaNotificationCallback callback,
            void* callback_arg,
            const std::map<std::string, CustomRecognizerSession>& custom_recognizers,
            const std::map<std::string, CustomActionSession>& custom_actions);
};

MAA_PROJECT_INTERFACE_NS_END
