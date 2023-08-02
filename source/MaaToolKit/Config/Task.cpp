#include "Task.h"

#include "Utils/Logger.hpp"

MAA_TOOLKIT_CONFIG_NS_BEGIN

void Task::set_name(MaaString new_name)
{
    LogInfo << VAR(name_) << VAR(new_name);
    name_ = new_name;
}

void Task::set_description(MaaString new_description)
{
    LogInfo << VAR(name_) << VAR(description_) << VAR(new_description);
    description_ = new_description;
}

void Task::set_type(MaaString new_type)
{
    LogInfo << VAR(name_) << VAR(type_) << VAR(new_type);
    type_ = new_type;
}

void Task::set_param(MaaJsonString new_param)
{
    LogInfo << VAR(name_) << VAR(param_) << VAR(new_param);
    param_ = new_param;
}

void Task::set_enabled(MaaBool new_enabled)
{
    LogInfo << VAR(name_) << VAR(enabled_) << VAR(new_enabled);
    enabled_ = new_enabled;
}

MaaStatus Task::status() const
{
    return MaaStatus();
}

std::ostream& operator<<(std::ostream& os, const Task& task)
{
    // TODO: 在此处插入 return 语句
}

MAA_TOOLKIT_CONFIG_NS_END
