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
    Config();
    virtual ~Config() override = default;

public: // from MaaToolKitConfigAPI
    virtual MaaString get_name() const override { return name_.c_str(); }
    virtual void set_name(MaaString new_name) override;
    virtual MaaString get_description() const override { return description_.c_str(); }
    virtual void set_description(MaaString new_description) override;
    virtual MaaString get_adb_path() const override { return adb_path_.c_str(); }
    virtual void set_adb_path(MaaString new_path) override;
    virtual MaaString get_adb_serial() const override { return adb_serial_.c_str(); }
    virtual void set_adb_serial(MaaString new_serial) override;
    virtual MaaJsonString get_adb_config() const override { return adb_config_.c_str(); }
    virtual void set_adb_config(MaaJsonString new_config) override;

    virtual MaaSize task_size() const override;
    virtual MaaToolKitTaskHandle task_by_index(MaaSize index) override;

    virtual MaaToolKitTaskHandle add_task(MaaString task_name, MaaToolKitTaskHandle copy_from) override;
    virtual void del_task(MaaString task_name) override;
    virtual bool set_task_index(MaaString task_name, MaaSize new_index) override;

    virtual void post_all_task() override;
    virtual MaaStatus wait_all_task() const override;
    virtual void stop_all_task() override;

    virtual MaaResourceHandle raw_resource() override { return resource_; }
    virtual MaaControllerHandle raw_controller() override { return controller_; }
    virtual MaaInstanceHandle raw_instance() override { return instance_; }

public:
    json::value to_json() const;
    bool from_json(const json::value& json);

private:
    std::string name_;
    std::string description_;
    std::string adb_path_;
    std::string adb_serial_;
    std::string adb_config_;

    std::vector<std::shared_ptr<Task>> task_vec_; // for C API
    std::map<std::string, std::shared_ptr<Task>> task_map_;

    MaaResourceHandle resource_ = nullptr;
    MaaControllerHandle controller_ = nullptr;
    MaaInstanceHandle instance_ = nullptr;
};

std::ostream& operator<<(std::ostream& os, const Config& config);

MAA_TOOLKIT_CONFIG_NS_END
