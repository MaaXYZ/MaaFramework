#pragma once

#include <string_view>

#include "Conf/Conf.h"
#include "MaaToolKit/MaaToolKitDef.h"

struct MaaToolKitConfigMgrAPI
{
    virtual ~MaaToolKitConfigMgrAPI() = default;

    virtual bool init() = 0;
    virtual bool uninit() = 0;

    virtual std::string_view get_custom_info(std::string_view key) const = 0;
    virtual void set_custom_info(std::string key, std::string value) = 0;

    virtual size_t config_size() const = 0;
    virtual MaaToolKitConfigHandle config_by_index(size_t index) = 0;
    virtual MaaToolKitConfigHandle current() = 0;

    virtual MaaToolKitConfigHandle add_config(std::string_view config_name, MaaToolKitConfigHandle copy_from) = 0;
    virtual bool del_config(std::string_view config_name) = 0;
    virtual bool set_current_config(std::string_view config_name) = 0;
};

struct MaaToolKitConfigAPI
{
    virtual ~MaaToolKitConfigAPI() = default;

    virtual std::string_view get_name() const = 0;
    virtual void set_name(std::string_view new_name) = 0;
    virtual std::string_view get_description() const = 0;
    virtual void set_description(std::string_view new_description) = 0;
    virtual std::string_view get_custom_info(std::string_view key) const = 0;
    virtual void set_custom_info(std::string key, std::string value) = 0;

    virtual size_t task_size() const = 0;
    virtual MaaToolKitTaskHandle task_by_index(size_t index) = 0;

    virtual MaaToolKitTaskHandle add_task(std::string_view task_name, MaaToolKitTaskHandle copy_from) = 0;
    virtual bool del_task(std::string_view task_name) = 0;
    virtual bool set_task_index(std::string_view task_name, size_t new_index) = 0;

    virtual void bind_instance(MaaInstanceHandle instance) = 0;
    virtual bool post_all_task() = 0;
    virtual MaaStatus wait_all_task() const = 0;
    virtual void stop_all_task() = 0;

    virtual MaaResourceHandle raw_resource() = 0;
    virtual MaaControllerHandle raw_controller() = 0;
    virtual MaaInstanceHandle raw_instance() = 0;
};

struct MaaToolKitTaskAPI
{
    virtual ~MaaToolKitTaskAPI() = default;

    virtual std::string_view get_name() const = 0;
    virtual void set_name(std::string_view new_name) = 0;
    virtual std::string_view get_description() const = 0;
    virtual void set_description(std::string_view new_description) = 0;
    virtual std::string_view get_entry() const = 0;
    virtual void set_entry(std::string_view new_entry) = 0;
    virtual std::string_view get_param() const = 0;
    virtual void set_param(std::string_view new_param) = 0;
    virtual bool get_enabled() const = 0;
    virtual void set_enabled(bool new_enabled) = 0;
    virtual std::string_view get_custom_info(std::string_view key) const = 0;
    virtual void set_custom_info(std::string key, std::string value) = 0;

    virtual MaaStatus status() const = 0;
};
