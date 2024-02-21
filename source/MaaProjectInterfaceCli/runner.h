#pragma once

#include "ProjectInterface/Types.h"

#include "MaaFramework/MaaDef.h"

class Runner
{
public:
    static bool run(const MAA_PROJECT_INTERFACE_NS::RuntimeParam& param);

private:
    static void on_maafw_notify(MaaStringView msg, MaaStringView details_json, MaaTransparentArg callback_arg);
};
