#pragma once

#include "Common/MaaConf.h"

#include <string_view>

MAA_TASK_NS_BEGIN

class AbstractTask
{
public:
    virtual ~AbstractTask() = default;

    virtual bool run() = 0;

    virtual std::string_view info() const = 0;
};

MAA_TASK_NS_END
