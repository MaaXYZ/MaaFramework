#pragma once

#include <map>
#include <ostream>
#include <string>
#include <vector>

#include <meojson/json.hpp>

#include "Conf/Conf.h"
#include "ConfigAPI.h"
#include "Task.h"

MAA_TOOLKIT_CONFIG_NS_BEGIN

class Config : public MaaToolKitConfigAPI
{
public:
    inline static const std::string kNameKey = "name";
    inline static const std::string kDescriptionKey = "description";
    inline static const std::string kAdbPathKey = "adb_path";
    inline static const std::string kAdbSerialKey = "adb_serial";
    inline static const std::string kAdbConfigKey = "adb_config";

    inline static const std::string kTasksKey = "tasks";

public:
    Config() = default;
    virtual ~Config() override = default;

public: // from MaaToolKitConfigAPI
    virtual std::string_view get_name() const override { return name_; }
    virtual void set_name(std::string_view new_name) override;
    virtual std::string_view get_description() const override { return description_; }
    virtual void set_description(std::string_view new_description) override;

    virtual void bind_instance(MaaInstanceHandle instance) override;

    virtual size_t task_size() const override;
    virtual MaaToolKitTaskHandle task_by_index(size_t index) override;

    virtual MaaToolKitTaskHandle add_task(std::string_view task_name, MaaToolKitTaskHandle copy_from) override;
    virtual bool del_task(std::string_view task_name) override;
    virtual bool set_task_index(std::string_view task_name, size_t new_index) override;

    virtual bool post_all_task() override;
    virtual MaaStatus wait_all_task() const override;
    virtual void stop_all_task() override;

    virtual MaaInstanceHandle raw_instance() override { return instance_; }
    virtual MaaResourceHandle raw_resource() override;
    virtual MaaControllerHandle raw_controller() override;

public:
    json::value to_json() const;
    bool from_json(const json::value& json);
    std::shared_ptr<Task> insert(std::string name, Task task);

    friend std::ostream& operator<<(std::ostream& os, const Config& config);

private:
    std::string name_;
    std::string description_;

    std::vector<std::shared_ptr<Task>> task_vec_; // for C API
    std::map<std::string, std::shared_ptr<Task>> task_map_;

    MaaInstanceHandle instance_ = nullptr;
    MaaTaskId last_task_id_ = 0;
};

MAA_TOOLKIT_CONFIG_NS_END
