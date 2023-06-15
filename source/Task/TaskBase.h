#pragma once

#include "Common/MaaConf.h"

#include <meojson/json.hpp>

#include <string_view>

MAA_TASK_NS_BEGIN

class TaskBase
{
public:
    virtual ~TaskBase() = default;

    virtual bool run() = 0;
    virtual bool set_param(const json::value& param) = 0;

    virtual std::string_view type() const = 0;
};

MAA_TASK_NS_END
