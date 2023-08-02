#pragma once

#include "Conf/Conf.h"
#include "MaaToolKit/MaaToolKitDef.h"

MAA_TOOLKIT_CONFIG_NS_BEGIN

struct MaaToolKitConfigAPI
{
    virtual MaaString get_name() const = 0;
    virtual bool set_name(MaaString new_name) = 0;
    virtual MaaString get_description() const = 0;
    virtual bool set_description(MaaString new_description) = 0;
    virtual MaaString get_adb_path() const = 0;
    virtual bool set_adb_path(MaaString new_path) = 0;
    virtual MaaString get_adb_serial() const = 0;
    virtual bool set_adb_serial(MaaString new_serial) = 0;
    virtual MaaJsonString get_adb_config() const = 0;
    virtual bool set_adb_serial(MaaJsonString new_config) = 0;

    virtual MaaSize task_size() const = 0;
    virtual MaaToolKitTaskHandle task_by_index(MaaSize index) const = 0;

    virtual MaaToolKitTaskHandle add_task(MaaString task_name, MaaToolKitTaskHandle copy_from) = 0;
    virtual bool del_task(MaaString task_name) = 0;

    virtual bool post_all_task() = 0;
    virtual MaaStatus wait_all_task() const = 0;
    virtual bool stop_all_task() = 0;

    virtual MaaResourceHandle raw_resource() = 0;
    virtual MaaControllerHandle raw_controller() = 0;
    virtual MaaInstanceHandle raw_instance() = 0;
};

struct MaaToolKitTaskAPI
{
    virtual MaaString get_name() const = 0;
    virtual bool set_name(MaaString new_name) = 0;
    virtual MaaString get_description() const = 0;
    virtual bool set_description(MaaString new_description) = 0;
    virtual MaaString get_type() const = 0;
    virtual bool set_type(MaaString new_type) = 0;
    virtual MaaJsonString get_param() const = 0;
    virtual bool set_param(MaaJsonString new_param) = 0;
    virtual MaaBool get_enabled() const = 0;
    virtual bool set_enabled(MaaBool new_enabled) = 0;
    virtual MaaSize get_index() const = 0;
    virtual bool set_index(MaaSize index) = 0;

    virtual MaaStatus status() const = 0;
};

MAA_TOOLKIT_CONFIG_NS_END
