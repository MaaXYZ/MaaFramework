#pragma once

#include "MaaToolKitDef.h"

#ifdef __cplusplus
extern "C"
{
#endif

    MaaSize MAA_TOOLKIT_API MaaToolKitConfigSize();
    MaaToolKitConfigHandle MAA_TOOLKIT_API MaaToolKitGetConfig(MaaSize index);
    MaaSize MAA_TOOLKIT_API MaaToolKitGetCurrentConfigIndex();

    MaaToolKitConfigHandle MAA_TOOLKIT_API MaaToolKitGetCurrentConfig();
    MaaBool MAA_TOOLKIT_API MaaToolKitSetCurrentConfig(MaaToolKitConfigHandle config_handle);

    MaaToolKitConfigHandle MAA_TOOLKIT_API MaaToolKitAddConfig(MaaString config_name, MaaString copy_from);
    MaaBool MAA_TOOLKIT_API MaaToolKitDelConfig(MaaString config_name);

    MaaString MAA_TOOLKIT_API MaaToolKitGetConfigName(MaaToolKitConfigHandle config_handle);
    MaaBool MAA_TOOLKIT_API MaaToolKitSetConfigName(MaaToolKitConfigHandle config_handle, MaaString new_name);
    MaaString MAA_TOOLKIT_API MaaToolKitGetAdbPath(MaaToolKitConfigHandle config_handle);
    MaaBool MAA_TOOLKIT_API MaaToolKitSetAdbPath(MaaToolKitConfigHandle config_handle, MaaString new_path);
    MaaString MAA_TOOLKIT_API MaaToolKitGetAdbSerial(MaaToolKitConfigHandle config_handle);
    MaaBool MAA_TOOLKIT_API MaaToolKitSetAdbSerial(MaaToolKitConfigHandle config_handle, MaaString new_serial);

    MaaSize MAA_TOOLKIT_API MaaToolKitTaskSize(MaaToolKitConfigHandle config_handle);
    MaaToolKitTaskHandle MAA_TOOLKIT_API MaaToolKitGetTask(MaaToolKitConfigHandle config_handle, MaaSize index);

    MaaToolKitTaskHandle MAA_TOOLKIT_API MaaToolKitAddTask(MaaToolKitConfigHandle config_handle, MaaString task_name,
                                                           MaaString copy_from);
    MaaBool MAA_TOOLKIT_API MaaToolKitDelTask(MaaToolKitConfigHandle config_handle, MaaString task_name);

    MaaString MAA_TOOLKIT_API MaaToolKitGetTaskName(MaaToolKitTaskHandle task_handle);
    MaaBool MAA_TOOLKIT_API MaaToolKitSetTaskName(MaaToolKitTaskHandle task_handle, MaaString new_name);
    MaaString MAA_TOOLKIT_API MaaToolKitGetTaskType(MaaToolKitTaskHandle task_handle);
    MaaBool MAA_TOOLKIT_API MaaToolKitSetTaskType(MaaToolKitTaskHandle task_handle, MaaString new_type);
    MaaString MAA_TOOLKIT_API MaaToolKitGetTaskParam(MaaToolKitTaskHandle task_handle);
    MaaBool MAA_TOOLKIT_API MaaToolKitSetTaskParam(MaaToolKitTaskHandle task_handle, MaaString new_param);
    MaaBool MAA_TOOLKIT_API MaaToolKitGetTaskEnabled(MaaToolKitTaskHandle task_handle);
    MaaBool MAA_TOOLKIT_API MaaToolKitSetTaskEnabled(MaaToolKitTaskHandle task_handle, MaaBool new_enabled);
    MaaBool MAA_TOOLKIT_API MaaToolKitGetTaskIndex(MaaToolKitTaskHandle task_handle);
    MaaBool MAA_TOOLKIT_API MaaToolKitSetTaskIndex(MaaToolKitTaskHandle task_handle, MaaBool new_enabled);

    MaaBool MAA_TOOLKIT_API MaaToolKitPostTask(MaaToolKitConfigHandle config_handle);
    MaaStatus MAA_TOOLKIT_API MaaToolKitWaitTask(MaaToolKitConfigHandle config_handle);
    MaaStatus MAA_TOOLKIT_API MaaToolKitTaskStatus(MaaToolKitConfigHandle config_handle);
    MaaBool MAA_TOOLKIT_API MaaToolKitStopTask(MaaToolKitConfigHandle config_handle);

    MaaResourceHandle MAA_TOOLKIT_API MaaToolKitGetFrameworkResource(MaaToolKitConfigHandle config_handle);
    MaaControllerHandle MAA_TOOLKIT_API MaaToolKitGetFrameworkController(MaaToolKitConfigHandle config_handle);
    MaaInstanceHandle MAA_TOOLKIT_API MaaToolKitGetFrameworkInstance(MaaToolKitConfigHandle config_handle);

    MaaBool MAA_TOOLKIT_API MaaToolKitStartWebServer(MaaString ip, uint16_t port);
    MaaBool MAA_TOOLKIT_API MaaToolKitStopWebServer();

#ifdef __cplusplus
}
#endif
