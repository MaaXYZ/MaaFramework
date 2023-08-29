#pragma once

#include "MaaToolKitDef.h"

#ifdef __cplusplus
extern "C"
{
#endif

    MaaBool MAA_TOOLKIT_API MaaToolKitInit();
    MaaBool MAA_TOOLKIT_API MaaToolKitUninit();

    /* device */

    MaaSize MAA_TOOLKIT_API MaaToolKitFindDevice();
    MaaSize MAA_TOOLKIT_API MaaToolKitFindDeviceBySpecifiedADB(MaaString adb_path);
    MaaString MAA_TOOLKIT_API MaaToolKitGetDeviceName(MaaSize index);
    MaaString MAA_TOOLKIT_API MaaToolKitGetDeviceAdbPath(MaaSize index);
    MaaString MAA_TOOLKIT_API MaaToolKitGetDeviceAdbSerial(MaaSize index);
    MaaAdbControllerType MAA_TOOLKIT_API MaaToolKitGetDeviceAdbControllerType(MaaSize index);
    MaaJsonString MAA_TOOLKIT_API MaaToolKitGetDeviceAdbConfig(MaaSize index);

    /* config */

    MaaSize MAA_TOOLKIT_API MaaToolKitConfigSize();
    MaaToolKitConfigHandle MAA_TOOLKIT_API MaaToolKitGetConfig(MaaSize index);

    MaaToolKitConfigHandle MAA_TOOLKIT_API MaaToolKitCurrentConfig();

    MaaBool MAA_TOOLKIT_API MaaToolKitBindInstance(MaaToolKitConfigHandle config_handle,
                                                   MaaInstanceHandle instance_handle);
    MaaToolKitConfigHandle MAA_TOOLKIT_API MaaToolKitAddConfig(MaaString config_name, MaaToolKitConfigHandle copy_from);
    MaaBool MAA_TOOLKIT_API MaaToolKitDelConfig(MaaString config_name);
    MaaBool MAA_TOOLKIT_API MaaToolKitSetCurrentConfig(MaaString config_name);

    MaaString MAA_TOOLKIT_API MaaToolKitConfigName(MaaToolKitConfigHandle config_handle);
    MaaString MAA_TOOLKIT_API MaaToolKitGetConfigDescription(MaaToolKitConfigHandle config_handle);
    MaaBool MAA_TOOLKIT_API MaaToolKitSetConfigDescription(MaaToolKitConfigHandle config_handle,
                                                           MaaString new_description);

    MaaSize MAA_TOOLKIT_API MaaToolKitTaskSize(MaaToolKitConfigHandle config_handle);
    MaaToolKitTaskHandle MAA_TOOLKIT_API MaaToolKitGetTask(MaaToolKitConfigHandle config_handle, MaaSize index);

    MaaToolKitTaskHandle MAA_TOOLKIT_API MaaToolKitAddTask(MaaToolKitConfigHandle config_handle, MaaString task_name,
                                                           MaaToolKitTaskHandle copy_from);
    MaaBool MAA_TOOLKIT_API MaaToolKitDelTask(MaaToolKitConfigHandle config_handle, MaaString task_name);
    MaaBool MAA_TOOLKIT_API MaaToolKitSetTaskIndex(MaaToolKitConfigHandle config_handle, MaaString task_name,
                                                   MaaSize new_index);

    MaaString MAA_TOOLKIT_API MaaToolKitTaskName(MaaToolKitTaskHandle task_handle);
    MaaString MAA_TOOLKIT_API MaaToolKitGetTaskDescription(MaaToolKitTaskHandle config_handle);
    MaaBool MAA_TOOLKIT_API MaaToolKitSetTaskDescription(MaaToolKitTaskHandle config_handle, MaaString new_description);
    MaaString MAA_TOOLKIT_API MaaToolKitGetTaskEntry(MaaToolKitTaskHandle task_handle);
    MaaBool MAA_TOOLKIT_API MaaToolKitSetTaskEntry(MaaToolKitTaskHandle task_handle, MaaString new_entry);
    MaaJsonString MAA_TOOLKIT_API MaaToolKitGetTaskParam(MaaToolKitTaskHandle task_handle);
    MaaBool MAA_TOOLKIT_API MaaToolKitSetTaskParam(MaaToolKitTaskHandle task_handle, MaaJsonString new_param);
    MaaBool MAA_TOOLKIT_API MaaToolKitGetTaskEnabled(MaaToolKitTaskHandle task_handle);
    MaaBool MAA_TOOLKIT_API MaaToolKitSetTaskEnabled(MaaToolKitTaskHandle task_handle, MaaBool new_enabled);

    MaaBool MAA_TOOLKIT_API MaaToolKitPostAllTask(MaaToolKitConfigHandle config_handle);
    MaaStatus MAA_TOOLKIT_API MaaToolKitWaitAllTask(MaaToolKitConfigHandle config_handle);
    MaaBool MAA_TOOLKIT_API MaaToolKitStopAllTask(MaaToolKitConfigHandle config_handle);

    MaaStatus MAA_TOOLKIT_API MaaToolKitTaskStatus(MaaToolKitTaskHandle task_handle);

    MaaResourceHandle MAA_TOOLKIT_API MaaToolKitGetRawResource(MaaToolKitConfigHandle config_handle);
    MaaControllerHandle MAA_TOOLKIT_API MaaToolKitGetRawController(MaaToolKitConfigHandle config_handle);
    MaaInstanceHandle MAA_TOOLKIT_API MaaToolKitGetRawInstance(MaaToolKitConfigHandle config_handle);

    // MaaBool MAA_TOOLKIT_API MaaToolKitStartWebServer(MaaString ip, uint16_t port);
    // MaaBool MAA_TOOLKIT_API MaaToolKitStopWebServer();

#ifdef __cplusplus
}
#endif
