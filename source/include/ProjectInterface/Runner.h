#pragma once

#include "Types.h"

#include "MaaFramework/MaaDef.h"

#include "Common/Conf.h"

MAA_PROJECT_INTERFACE_NS_BEGIN

class Runner
{
public:
    static bool
        run(const RuntimeParam& param,
            const std::map<std::string, CustomRecognitionSession>& custom_recognitions,
            const std::map<std::string, CustomActionSession>& custom_actions);
};

MAA_PROJECT_INTERFACE_NS_END
