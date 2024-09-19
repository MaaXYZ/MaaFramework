#pragma once

#include "Types.h"

#include "MaaFramework/MaaDef.h"

MAA_PROJECT_INTERFACE_NS_BEGIN

class Runner
{
public:
    static bool
        run(const RuntimeParam& param,
            MaaNotificationCallback notify,
            void* notify_trans_arg,
            const std::map<std::string, CustomRecognitionSession>& custom_recognitions,
            const std::map<std::string, CustomActionSession>& custom_actions);
};

MAA_PROJECT_INTERFACE_NS_END
