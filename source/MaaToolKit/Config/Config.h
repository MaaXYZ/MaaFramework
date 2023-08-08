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
    virtual ~Config() override;

public: // from MaaToolKitConfigAPI
    virtual std::string_view get_name() const override { return name_; }
    virtual void set_name(std::string_view new_name) override;
    virtual std::string_view get_description() const override { return description_; }
    virtual void set_description(std::string_view new_description) override;
    virtual std::string_view get_adb_path() const override { return adb_path_; }
    virtual void set_adb_path(std::string_view new_path) override;
    virtual std::string_view get_adb_serial() const override { return adb_serial_; }
    virtual void set_adb_serial(std::string_view new_serial) override;
    virtual std::string_view get_adb_config() const override { return adb_config_; }
    virtual void set_adb_config(std::string_view new_config) override;

    virtual size_t task_size() const override;
    virtual MaaToolKitTaskHandle task_by_index(size_t index) override;

    virtual MaaToolKitTaskHandle add_task(std::string_view task_name, MaaToolKitTaskHandle copy_from) override;
    virtual bool del_task(std::string_view task_name) override;
    virtual bool set_task_index(std::string_view task_name, size_t new_index) override;

    virtual bool post_all_task() override;
    virtual MaaStatus wait_all_task() const override;
    virtual void stop_all_task() override;

    virtual MaaResourceHandle raw_resource() override { return resource_; }
    virtual MaaControllerHandle raw_controller() override { return controller_; }
    virtual MaaInstanceHandle raw_instance() override { return instance_; }

public:
    bool init();
    void uninit();

    json::value to_json() const;
    bool from_json(const json::value& json);
    std::shared_ptr<Task> insert(std::string name, Task task);

    friend std::ostream& operator<<(std::ostream& os, const Config& config);

private:
    std::string name_;
    std::string description_;
    std::string adb_path_;
    std::string adb_serial_;
    int32_t adb_type_ = 0; // TODO
    std::string adb_config_;

    std::vector<std::shared_ptr<Task>> task_vec_; // for C API
    std::map<std::string, std::shared_ptr<Task>> task_map_;

    MaaResourceHandle resource_ = nullptr;
    MaaControllerHandle controller_ = nullptr;
    MaaInstanceHandle instance_ = nullptr;

    std::vector<std::filesystem::path> resource_paths_;
};

MAA_TOOLKIT_CONFIG_NS_END
