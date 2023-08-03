#pragma once

#include "Conf/Conf.h"
#include "MaaToolKit/MaaToolKitDef.h"

struct MaaToolKitConfigMgrAPI
{
    virtual ~MaaToolKitConfigMgrAPI() = default;

    virtual bool init() = 0;
    virtual bool uninit() = 0;

    virtual MaaSize config_size() const = 0;
    virtual MaaToolKitConfigHandle config_by_index(MaaSize index) = 0;
    virtual MaaToolKitConfigHandle current() = 0;

    virtual MaaToolKitConfigHandle add_config(MaaString config_name, MaaToolKitConfigHandle copy_from) = 0;
    virtual void del_task(MaaString config_name) = 0;
    virtual bool set_current_config(MaaString config_name) = 0;
};

struct MaaToolKitConfigAPI
{
    virtual ~MaaToolKitConfigAPI() = default;

    virtual MaaString get_name() const = 0;
    virtual void set_name(MaaString new_name) = 0;
    virtual MaaString get_description() const = 0;
    virtual void set_description(MaaString new_description) = 0;
    virtual MaaString get_adb_path() const = 0;
    virtual void set_adb_path(MaaString new_path) = 0;
    virtual MaaString get_adb_serial() const = 0;
    virtual void set_adb_serial(MaaString new_serial) = 0;
    virtual MaaJsonString get_adb_config() const = 0;
    virtual void set_adb_config(MaaJsonString new_config) = 0;

    virtual MaaSize task_size() const = 0;
    virtual MaaToolKitTaskHandle task_by_index(MaaSize index) = 0;

    virtual MaaToolKitTaskHandle add_task(MaaString task_name, MaaToolKitTaskHandle copy_from) = 0;
    virtual void del_task(MaaString task_name) = 0;
    virtual bool set_task_index(MaaString task_name, MaaSize new_index) = 0;

    virtual void post_all_task() = 0;
    virtual MaaStatus wait_all_task() const = 0;
    virtual void stop_all_task() = 0;

    virtual MaaResourceHandle raw_resource() = 0;
    virtual MaaControllerHandle raw_controller() = 0;
    virtual MaaInstanceHandle raw_instance() = 0;
};

struct MaaToolKitTaskAPI
{
    virtual ~MaaToolKitTaskAPI() = default;

    virtual MaaString get_name() const = 0;
    virtual void set_name(MaaString new_name) = 0;
    virtual MaaString get_description() const = 0;
    virtual void set_description(MaaString new_description) = 0;
    virtual MaaString get_type() const = 0;
    virtual void set_type(MaaString new_type) = 0;
    virtual MaaJsonString get_param() const = 0;
    virtual void set_param(MaaJsonString new_param) = 0;
    virtual MaaBool get_enabled() const = 0;
    virtual void set_enabled(MaaBool new_enabled) = 0;

    virtual MaaStatus status() const = 0;
};
