#pragma once

#include "Types.h"

#include "Common/Conf.h"

MAA_PROJECT_INTERFACE_NS_BEGIN

class Runner
{
public:
    static bool run(const RuntimeParam& param, int progress_level = 1);
};

MAA_PROJECT_INTERFACE_NS_END
