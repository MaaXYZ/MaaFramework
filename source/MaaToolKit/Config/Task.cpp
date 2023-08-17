#include "Task.h"

#include "Utils/Logger.h"

MAA_TOOLKIT_CONFIG_NS_BEGIN

void Task::set_name(std::string_view new_name)
{
    LogInfo << VAR(name_) << VAR(new_name);
    name_ = new_name;
}

void Task::set_description(std::string_view new_description)
{
    LogInfo << VAR(name_) << VAR(description_) << VAR(new_description);
    description_ = new_description;
}

void Task::set_entry(std::string_view new_entry)
{
    LogInfo << VAR(name_) << VAR(entry_) << VAR(new_entry);
    entry_ = new_entry;
}

void Task::set_param(std::string_view new_param)
{
    LogInfo << VAR(name_) << VAR(param_) << VAR(new_param);
    param_ = new_param;
}

void Task::set_enabled(bool new_enabled)
{
    LogInfo << VAR(name_) << VAR(enabled_) << VAR(new_enabled);
    enabled_ = new_enabled;
}

MaaStatus Task::status() const
{
    return MaaStatus();
}

json::value Task::to_json() const
{
    json::value root;
    root[kNameKey] = name_;
    root[kDescriptionKey] = description_;
    root[kEntryKey] = entry_;
    root[kParamKey] = param_;
    root[kEnabledKey] = enabled_;

    return root;
}

bool Task::from_json(const json::value& json)
{
    LogFunc << VAR(json);

    auto name_opt = json.find<std::string>(kNameKey);
    if (!name_opt) {
        return false;
    }
    name_ = std::move(name_opt).value();

    description_ = json.get(kDescriptionKey, std::string());
    entry_ = json.get(kEntryKey, std::string());
    param_ = json.get(kParamKey, std::string());
    enabled_ = json.get(kEnabledKey, true);

    return true;
}

std::ostream& operator<<(std::ostream& os, const Task& task)
{
    os << VAR_RAW(task.name_) << VAR_RAW(task.description_) << VAR_RAW(task.entry_) << VAR_RAW(task.param_)
       << VAR_RAW(task.enabled_);
    return os;
}

MAA_TOOLKIT_CONFIG_NS_END
