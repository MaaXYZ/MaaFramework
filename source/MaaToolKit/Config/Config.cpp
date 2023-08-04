#include "Config.h"

#include "MaaFramework/MaaAPI.h"

#include "Utils/Logger.hpp"
#include "Utils/Ranges.hpp"

MAA_TOOLKIT_CONFIG_NS_BEGIN

Config::Config()
{
    LogFunc;
    // TODO
}

void Config::set_name(std::string_view new_name)
{
    LogInfo << VAR(name_) << VAR(new_name);
    name_ = new_name;
}

void Config::set_description(std::string_view new_description)
{
    LogInfo << VAR(name_) << VAR(description_) << VAR(new_description);
    description_ = new_description;
}

void Config::set_adb_path(std::string_view new_path)
{
    LogInfo << VAR(name_) << VAR(adb_path_) << VAR(new_path);
    adb_path_ = new_path;
}

void Config::set_adb_serial(std::string_view new_serial)
{
    LogInfo << VAR(name_) << VAR(adb_serial_) << VAR(new_serial);
    adb_serial_ = new_serial;
}

void Config::set_adb_config(std::string_view new_config)
{
    LogInfo << VAR(name_) << VAR(adb_config_) << VAR(new_config);
    adb_config_ = new_config;
}

size_t Config::task_size() const
{
    return task_vec_.size();
}

MaaToolKitTaskHandle Config::task_by_index(size_t index)
{
    if (index >= task_vec_.size()) {
        LogError << "Out of range" << VAR(index) << VAR(task_vec_.size());
        return nullptr;
    }
    return task_vec_[index].get();
}

MaaToolKitTaskHandle Config::add_task(std::string_view task_name, MaaToolKitTaskHandle copy_from)
{
    LogInfo << VAR(name_) << VAR(task_name) << VAR(copy_from);

    std::string str_task_name(task_name);
    if (task_map_.contains(str_task_name)) {
        LogError << "Task name already exists" << VAR(task_name) << VAR(task_map_);
        return nullptr;
    }

    Task new_task;
    if (auto cast = dynamic_cast<Task*>(copy_from); cast) {
        new_task = *cast;
    }
    new_task.set_name(task_name);

    auto new_task_ptr = std::make_shared<Task>(std::move(new_task));
    auto& ref = task_vec_.emplace_back(new_task_ptr);
    task_map_.emplace(task_name, new_task_ptr);

    LogTrace << VAR(task_name) << VAR(new_task_ptr) << VAR(*new_task_ptr) << VAR(task_vec_) << VAR(task_map_);

    return ref.get();
}

bool Config::del_task(std::string_view task_name)
{
    LogInfo << VAR(name_) << VAR(task_name);

    std::string str_task_name(task_name);
    bool removed = task_map_.erase(str_task_name) > 0;
    if (!removed) {
        LogError << "Task name not found in map" << VAR(task_name) << VAR(task_map_);
        return false;
    }
    auto find_it = MAA_RNS::ranges::find_if(task_vec_, [&](const auto& task) { return task->get_name() == task_name; });
    if (find_it == task_vec_.end()) {
        LogError << "Task name not found in vec" << VAR(task_name) << VAR(task_vec_);
        return false;
    }
    task_vec_.erase(find_it);

    LogTrace << VAR(task_name) << VAR(task_vec_) << VAR(task_map_);
    return true;
}

bool Config::set_task_index(std::string_view task_name, size_t new_index)
{
    LogInfo << VAR(name_) << VAR(task_name) << VAR(new_index);

    auto find_it = MAA_RNS::ranges::find_if(task_vec_, [&](const auto& task) { return task->get_name() == task_name; });
    if (find_it == task_vec_.end()) {
        LogError << "Task name not found in vec:" << task_name;
        return false;
    }
    if (new_index >= task_vec_.size()) {
        LogError << "Out of range" << VAR(new_index) << VAR(task_vec_.size());
        return false;
    }

    auto task_ptr = std::move(*find_it);
    task_vec_.erase(find_it);
    task_vec_.insert(task_vec_.begin() + new_index, std::move(task_ptr));

    return true;
}

bool Config::post_all_task()
{
    // TODO
    return false;
}

MaaStatus Config::wait_all_task() const
{
    return MaaStatus();
}

void Config::stop_all_task() {}

json::value Config::to_json() const
{
    return json::value();
}

bool Config::from_json(const json::value& json)
{
    std::ignore = json;
    return false;
}

std::ostream& operator<<(std::ostream& os, const Config& config)
{
    // TODO: 在此处插入 return 语句
    std::ignore = config;
    return os;
}

MAA_TOOLKIT_CONFIG_NS_END
