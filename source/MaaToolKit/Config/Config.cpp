#include "Config.h"

#include "MaaFramework/MaaAPI.h"

#include "Utils/Logger.hpp"
#include "Utils/Ranges.hpp"

MAA_TOOLKIT_CONFIG_NS_BEGIN

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

void Config::bind_instance(MaaInstanceHandle instance)
{
    LogInfo << VAR(name_) << VAR_VOIDP(instance_) << VAR_VOIDP(instance);
    instance_ = instance;
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

    return insert(std::move(str_task_name), std::move(new_task)).get();
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
    if (!instance_) {
        LogError << "Instance not binded";
        return false;
    }

    for (const auto& task : task_vec_) {
        if (!task->get_enabled()) {
            continue;
        }

        last_task_id_ = MaaPostTask(instance_, task->get_entry().data(), task->get_param().data());
    }
}

MaaStatus Config::wait_all_task() const
{
    if (!instance_) {
        LogError << "Instance not binded";
        return MaaStatus_Invalid;
    }

    return MaaWaitTask(instance_, last_task_id_);
}

void Config::stop_all_task()
{
    if (!instance_) {
        LogError << "Instance not binded";
        return;
    }

    MaaStop(instance_);
}

MaaResourceHandle Config::raw_resource()
{
    return MaaGetResource(instance_);
}

MaaControllerHandle Config::raw_controller()
{
    return MaaGetController(instance_);
}

json::value Config::to_json() const
{
    json::value root;
    root[kNameKey] = name_;
    root[kDescriptionKey] = description_;

    auto& tasks = root[kTasksKey].as_array();
    for (const auto& task : task_vec_) {
        tasks.emplace_back(task->to_json());
    }
    return root;
}

bool Config::from_json(const json::value& json)
{
    LogFunc << VAR(json);

    auto name_opt = json.find<std::string>(kNameKey);
    if (!name_opt) {
        return false;
    }
    name_ = std::move(name_opt).value();

    description_ = json.get(kDescriptionKey, std::string());

    for (const auto& j_task : json.get(kTasksKey, json::array())) {
        Task task;
        if (!task.from_json(j_task)) {
            LogError << "Task from json failed" << VAR(j_task);
            return false;
        }
        insert(std::string(task.get_name()), std::move(task));
    }
    return true;
}

std::shared_ptr<Task> Config::insert(std::string name, Task task)
{
    auto new_task_ptr = std::make_shared<Task>(std::move(task));
    task_vec_.emplace_back(new_task_ptr);
    task_map_.insert_or_assign(name, new_task_ptr);

    LogTrace << VAR(name) << VAR(new_task_ptr) << VAR(*new_task_ptr) << VAR(task_vec_) << VAR(task_map_);
    return new_task_ptr;
}

std::ostream& operator<<(std::ostream& os, const Config& config)
{
    os << VAR_RAW(config.name_) << VAR_RAW(config.description_) << VAR_RAW(config.task_vec_.size());
    return os;
}

MAA_TOOLKIT_CONFIG_NS_END
