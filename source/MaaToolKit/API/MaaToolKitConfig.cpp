#include "MaaToolKit/MaaToolKitAPI.h"

#include "Config/ConfigMgr.h"
#include "Utils/Logger.h"

static MaaToolKitConfigMgrAPI& config_mgr = MAA_TOOLKIT_CONFIG_NS::ConfigMgr::get_instance();

MaaBool MaaToolKitInit()
{
    LogFunc;

    return config_mgr.init();
}

MaaBool MaaToolKitUninit()
{
    LogFunc;

    return config_mgr.uninit();
}

MaaStringView MaaToolKitGetCustomInfo(MaaStringView key)
{
    return config_mgr.get_custom_info(key).data();
}

MaaBool MaaToolKitSetCustomInfo(MaaStringView key, MaaStringView value)
{
    config_mgr.set_custom_info(key, value);
    return true;
}

MaaSize MaaToolKitConfigSize()
{
    return config_mgr.config_size();
}

MaaToolKitConfigHandle MaaToolKitGetConfig(MaaSize index)
{
    return config_mgr.config_by_index(index);
}

MaaToolKitConfigHandle MaaToolKitCurrentConfig()
{
    return config_mgr.current();
}

MaaBool MaaToolKitBindInstance(MaaToolKitConfigHandle config_handle, MaaInstanceHandle instance_handle)
{
    if (!config_handle) {
        LogError << "config_handle is null" << VAR(config_handle);
        return false;
    }
    if (!instance_handle) {
        LogError << "instance_handle is null" << VAR(instance_handle);
        return false;
    }

    config_handle->bind_instance(instance_handle);
    return true;
}

MaaToolKitConfigHandle MaaToolKitAddConfig(MaaStringView config_name, MaaToolKitConfigHandle copy_from)
{
    return config_mgr.add_config(config_name, copy_from);
}

MaaBool MaaToolKitDelConfig(MaaStringView config_name)
{
    return config_mgr.del_config(config_name);
}

MaaBool MaaToolKitSetCurrentConfig(MaaStringView config_name)
{
    return config_mgr.set_current_config(config_name);
}

MaaStringView MaaToolKitConfigName(MaaToolKitConfigHandle config_handle)
{
    if (!config_handle) {
        LogError << "config_handle is null" << VAR(config_handle);
        return nullptr;
    }

    return config_handle->get_name().data();
}

MaaStringView MaaToolKitGetConfigDescription(MaaToolKitConfigHandle config_handle)
{
    if (!config_handle) {
        LogError << "config_handle is null" << VAR(config_handle);
        return nullptr;
    }

    return config_handle->get_description().data();
}

MaaBool MaaToolKitSetConfigDescription(MaaToolKitConfigHandle config_handle, MaaStringView new_description)
{
    if (!config_handle) {
        LogError << "config_handle is null" << VAR(config_handle);
        return false;
    }

    config_handle->set_description(new_description);
    return true;
}

MaaStringView MaaToolKitGetConfigCustomInfo(MaaToolKitConfigHandle config_handle, MaaStringView key)
{
    if (!config_handle) {
        LogError << "config_handle is null" << VAR(config_handle);
        return nullptr;
    }

    return config_handle->get_custom_info(key).data();
}

MaaBool MaaToolKitSetConfigCustomInfo(MaaToolKitConfigHandle config_handle, MaaStringView key, MaaStringView value)
{
    if (!config_handle) {
        LogError << "config_handle is null" << VAR(config_handle);
        return false;
    }

    config_handle->set_custom_info(key, value);
    return true;
}

MaaSize MaaToolKitTaskSize(MaaToolKitConfigHandle config_handle)
{
    if (!config_handle) {
        LogError << "config_handle is null" << VAR(config_handle);
        return 0;
    }

    return config_handle->task_size();
}

MaaToolKitTaskHandle MaaToolKitGetTask(MaaToolKitConfigHandle config_handle, MaaSize index)
{
    if (!config_handle) {
        LogError << "config_handle is null" << VAR(config_handle);
        return nullptr;
    }

    return config_handle->task_by_index(index);
}

MaaToolKitTaskHandle MaaToolKitAddTask(MaaToolKitConfigHandle config_handle, MaaStringView task_name,
                                       MaaToolKitTaskHandle copy_from)
{
    if (!config_handle) {
        LogError << "config_handle is null" << VAR(config_handle);
        return nullptr;
    }

    return config_handle->add_task(task_name, copy_from);
}

MaaBool MaaToolKitDelTask(MaaToolKitConfigHandle config_handle, MaaStringView task_name)
{
    if (!config_handle) {
        LogError << "config_handle is null" << VAR(config_handle);
        return false;
    }

    return config_handle->del_task(task_name);
}

MaaBool MaaToolKitSetTaskIndex(MaaToolKitConfigHandle config_handle, MaaStringView task_name, MaaSize new_index)
{
    if (!config_handle) {
        LogError << "config_handle is null" << VAR(config_handle);
        return false;
    }

    return config_handle->set_task_index(task_name, new_index);
}

MaaStringView MaaToolKitTaskName(MaaToolKitTaskHandle task_handle)
{
    if (!task_handle) {
        LogError << "task_handle is null" << VAR(task_handle);
        return nullptr;
    }

    return task_handle->get_name().data();
}

MaaStringView MaaToolKitGetTaskDescription(MaaToolKitTaskHandle task_handle)
{
    if (!task_handle) {
        LogError << "task_handle is null" << VAR(task_handle);
        return nullptr;
    }

    return task_handle->get_description().data();
}

MaaBool MaaToolKitSetTaskDescription(MaaToolKitTaskHandle task_handle, MaaStringView new_description)
{
    if (!task_handle) {
        LogError << "task_handle is null" << VAR(task_handle);
        return false;
    }

    task_handle->set_description(new_description);
    return true;
}

MaaStringView MaaToolKitGetTaskEntry(MaaToolKitTaskHandle task_handle)
{
    if (!task_handle) {
        LogError << "task_handle is null" << VAR(task_handle);
        return nullptr;
    }

    return task_handle->get_entry().data();
}

MaaBool MaaToolKitSetTaskEntry(MaaToolKitTaskHandle task_handle, MaaStringView new_entry)
{
    if (!task_handle) {
        LogError << "task_handle is null" << VAR(task_handle);
        return false;
    }

    task_handle->set_entry(new_entry);
    return true;
}

MaaStringView MaaToolKitGetTaskParam(MaaToolKitTaskHandle task_handle)
{
    if (!task_handle) {
        LogError << "task_handle is null" << VAR(task_handle);
        return nullptr;
    }
    return task_handle->get_param().data();
}

MaaBool MaaToolKitSetTaskParam(MaaToolKitTaskHandle task_handle, MaaStringView new_param)
{
    if (!task_handle) {
        LogError << "task_handle is null" << VAR(task_handle);
        return false;
    }
    task_handle->set_param(new_param);
    return true;
}

MaaBool MaaToolKitGetTaskEnabled(MaaToolKitTaskHandle task_handle)
{
    if (!task_handle) {
        LogError << "task_handle is null" << VAR(task_handle);
        return false;
    }
    return task_handle->get_enabled();
}

MaaBool MaaToolKitSetTaskEnabled(MaaToolKitTaskHandle task_handle, MaaBool new_enabled)
{
    if (!task_handle) {
        LogError << "task_handle is null" << VAR(task_handle);
        return false;
    }
    task_handle->set_enabled(new_enabled);
    return true;
}

MaaStringView MaaToolKitGetTaskCustomInfo(MaaToolKitTaskHandle task_handle, MaaStringView key)
{
    if (!task_handle) {
        LogError << "task_handle is null" << VAR(task_handle);
        return nullptr;
    }
    return task_handle->get_custom_info(key).data();
}

MaaBool MaaToolKitSetTaskCustomInfo(MaaToolKitTaskHandle task_handle, MaaStringView key, MaaStringView value)
{
    if (!task_handle) {
        LogError << "task_handle is null" << VAR(task_handle);
        return false;
    }
    task_handle->set_custom_info(key, value);
    return true;
}

MaaBool MaaToolKitPostAllTask(MaaToolKitConfigHandle config_handle)
{
    if (!config_handle) {
        LogError << "config_handle is null" << VAR(config_handle);
        return false;
    }
    return config_handle->post_all_task();
}

MaaStatus MaaToolKitWaitAllTask(MaaToolKitConfigHandle config_handle)
{
    if (!config_handle) {
        LogError << "config_handle is null" << VAR(config_handle);
        return false;
    }
    return config_handle->wait_all_task();
}

MaaBool MaaToolKitStopAllTask(MaaToolKitConfigHandle config_handle)
{
    if (!config_handle) {
        LogError << "config_handle is null" << VAR(config_handle);
        return false;
    }
    config_handle->stop_all_task();
    return true;
}

MaaStatus MaaToolKitTaskStatus(MaaToolKitTaskHandle task_handle)
{
    if (!task_handle) {
        LogError << "task_handle is null" << VAR(task_handle);
        return false;
    }
    return task_handle->status();
}

MaaResourceHandle MaaToolKitGetRawResource(MaaToolKitConfigHandle config_handle)
{
    if (!config_handle) {
        LogError << "config_handle is null" << VAR(config_handle);
        return nullptr;
    }
    return config_handle->raw_resource();
}

MaaControllerHandle MaaToolKitGetRawController(MaaToolKitConfigHandle config_handle)
{
    if (!config_handle) {
        LogError << "config_handle is null" << VAR(config_handle);
        return nullptr;
    }
    return config_handle->raw_controller();
}

MaaInstanceHandle MaaToolKitGetRawInstance(MaaToolKitConfigHandle config_handle)
{
    if (!config_handle) {
        LogError << "config_handle is null" << VAR(config_handle);
        return nullptr;
    }
    return config_handle->raw_instance();
}
