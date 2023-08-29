#include "MaaToolKit/MaaToolKitAPI.h"

#include "Config/ConfigMgr.h"
#include "Device/DeviceMgrLinux.h"
#include "Device/DeviceMgrMacOS.h"
#include "Device/DeviceMgrWin32.h"
#include "Server/HttpServer.h"
#include "Utils/Logger.h"

static MaaToolKitConfigMgrAPI& config_mgr = MAA_TOOLKIT_CONFIG_NS::ConfigMgr::get_instance();

#if defined(_WIN32)
static MaaToolKitDeviceMgrAPI& device_mgr = MAA_TOOLKIT_DEVICE_NS::DeviceMgrWin32::get_instance();
#elif defined(__linux__)
static MaaToolKitDeviceMgrAPI& device_mgr = MAA_TOOLKIT_DEVICE_NS::DeviceMgrLinux::get_instance();
#elif defined(__APPLE__)
static MaaToolKitDeviceMgrAPI& device_mgr = MAA_TOOLKIT_DEVICE_NS::DeviceMgrMacOS::get_instance();
#endif

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

MaaSize MaaToolKitFindDevice()
{
    LogFunc;

    return device_mgr.find_device();
}

MaaSize MaaToolKitFindDeviceBySpecifiedADB(MaaString adb_path)
{
    LogFunc;

    return device_mgr.find_device(adb_path);
}

MaaString MaaToolKitGetDeviceName(MaaSize index)
{
    return device_mgr.get_devices().at(index).name.c_str();
}

MaaString MaaToolKitGetDeviceAdbPath(MaaSize index)
{
    return device_mgr.get_devices().at(index).adb_path.c_str();
}

MaaString MaaToolKitDeviceAdbSerial(MaaSize index)
{
    return device_mgr.get_devices().at(index).adb_serial.c_str();
}

MaaAdbControllerType MaaToolKitGetDeviceAdbControllerType(MaaSize index)
{
    return device_mgr.get_devices().at(index).adb_controller_type;
}

MaaJsonString MaaToolKitGetDeviceAdbConfig(MaaSize index)
{
    return device_mgr.get_devices().at(index).adb_config.c_str();
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

MaaToolKitConfigHandle MaaToolKitAddConfig(MaaString config_name, MaaToolKitConfigHandle copy_from)
{
    return config_mgr.add_config(config_name, copy_from);
}

MaaBool MaaToolKitDelConfig(MaaString config_name)
{
    return config_mgr.del_config(config_name);
}

MaaBool MaaToolKitSetCurrentConfig(MaaString config_name)
{
    return config_mgr.set_current_config(config_name);
}

MaaString MaaToolKitConfigName(MaaToolKitConfigHandle config_handle)
{
    if (!config_handle) {
        LogError << "config_handle is null" << VAR(config_handle);
        return nullptr;
    }

    return config_handle->get_name().data();
}

MaaString MaaToolKitGetConfigDescription(MaaToolKitConfigHandle config_handle)
{
    if (!config_handle) {
        LogError << "config_handle is null" << VAR(config_handle);
        return nullptr;
    }

    return config_handle->get_description().data();
}

MaaBool MaaToolKitSetConfigDescription(MaaToolKitConfigHandle config_handle, MaaString new_description)
{
    if (!config_handle) {
        LogError << "config_handle is null" << VAR(config_handle);
        return false;
    }

    config_handle->set_description(new_description);
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

MaaToolKitTaskHandle MaaToolKitAddTask(MaaToolKitConfigHandle config_handle, MaaString task_name,
                                       MaaToolKitTaskHandle copy_from)
{
    if (!config_handle) {
        LogError << "config_handle is null" << VAR(config_handle);
        return nullptr;
    }

    return config_handle->add_task(task_name, copy_from);
}

MaaBool MaaToolKitDelTask(MaaToolKitConfigHandle config_handle, MaaString task_name)
{
    if (!config_handle) {
        LogError << "config_handle is null" << VAR(config_handle);
        return false;
    }

    return config_handle->del_task(task_name);
}

MaaBool MaaToolKitSetTaskIndex(MaaToolKitConfigHandle config_handle, MaaString task_name, MaaSize new_index)
{
    if (!config_handle) {
        LogError << "config_handle is null" << VAR(config_handle);
        return false;
    }

    return config_handle->set_task_index(task_name, new_index);
}

MaaString MaaToolKitTaskName(MaaToolKitTaskHandle task_handle)
{
    if (!task_handle) {
        LogError << "task_handle is null" << VAR(task_handle);
        return nullptr;
    }

    return task_handle->get_name().data();
}

MaaString MaaToolKitGetTaskDescription(MaaToolKitTaskHandle task_handle)
{
    if (!task_handle) {
        LogError << "task_handle is null" << VAR(task_handle);
        return nullptr;
    }

    return task_handle->get_description().data();
}

MaaBool MaaToolKitSetTaskDescription(MaaToolKitTaskHandle task_handle, MaaString new_description)
{
    if (!task_handle) {
        LogError << "task_handle is null" << VAR(task_handle);
        return false;
    }

    task_handle->set_description(new_description);
    return true;
}

MaaString MaaToolKitGetTaskEntry(MaaToolKitTaskHandle task_handle)
{
    if (!task_handle) {
        LogError << "task_handle is null" << VAR(task_handle);
        return nullptr;
    }

    return task_handle->get_entry().data();
}

MaaBool MaaToolKitSetTaskEntry(MaaToolKitTaskHandle task_handle, MaaString new_entry)
{
    if (!task_handle) {
        LogError << "task_handle is null" << VAR(task_handle);
        return false;
    }

    task_handle->set_entry(new_entry);
    return true;
}

MaaJsonString MaaToolKitGetTaskParam(MaaToolKitTaskHandle task_handle)
{
    if (!task_handle) {
        LogError << "task_handle is null" << VAR(task_handle);
        return nullptr;
    }
    return task_handle->get_param().data();
}

MaaBool MaaToolKitSetTaskParam(MaaToolKitTaskHandle task_handle, MaaJsonString new_param)
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

MaaBool MaaToolKitStartWebServer(MaaString ip, uint16_t port)
{
    return MAA_TOOLKIT_SERVER_NS::HttpServer::get_instance().start(ip, port);
}

MaaBool MaaToolKitStopWebServer()
{
    return MAA_TOOLKIT_SERVER_NS::HttpServer::get_instance().stop();
}
