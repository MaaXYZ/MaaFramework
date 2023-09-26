#pragma once

#include "../MaaToolKitDef.h"

#ifdef __cplusplus
extern "C"
{
#endif

    MaaBool MAA_TOOLKIT_API MaaToolKitInit();
    MaaBool MAA_TOOLKIT_API MaaToolKitUninit();

    MaaStringView MAA_TOOLKIT_API MaaToolKitGetCustomInfo(MaaStringView key);
    MaaBool MAA_TOOLKIT_API MaaToolKitSetCustomInfo(MaaStringView key, MaaStringView value);
    MaaSize MAA_TOOLKIT_API MaaToolKitConfigSize();
    MaaToolKitConfigHandle MAA_TOOLKIT_API MaaToolKitGetConfig(MaaSize index);

    MaaToolKitConfigHandle MAA_TOOLKIT_API MaaToolKitCurrentConfig();

    MaaBool MAA_TOOLKIT_API MaaToolKitBindInstance(MaaToolKitConfigHandle config_handle,
                                                   MaaInstanceHandle instance_handle);
    MaaToolKitConfigHandle MAA_TOOLKIT_API MaaToolKitAddConfig(MaaStringView config_name,
                                                               MaaToolKitConfigHandle copy_from);
    MaaBool MAA_TOOLKIT_API MaaToolKitDelConfig(MaaStringView config_name);
    MaaBool MAA_TOOLKIT_API MaaToolKitSetCurrentConfig(MaaStringView config_name);

    MaaStringView MAA_TOOLKIT_API MaaToolKitConfigName(MaaToolKitConfigHandle config_handle);
    MaaStringView MAA_TOOLKIT_API MaaToolKitGetConfigDescription(MaaToolKitConfigHandle config_handle);
    MaaBool MAA_TOOLKIT_API MaaToolKitSetConfigDescription(MaaToolKitConfigHandle config_handle,
                                                           MaaStringView new_description);
    MaaStringView MAA_TOOLKIT_API MaaToolKitGetConfigCustomInfo(MaaToolKitConfigHandle config_handle,
                                                                MaaStringView key);
    MaaBool MAA_TOOLKIT_API MaaToolKitSetConfigCustomInfo(MaaToolKitConfigHandle config_handle, MaaStringView key,
                                                          MaaStringView value);

    MaaSize MAA_TOOLKIT_API MaaToolKitTaskSize(MaaToolKitConfigHandle config_handle);
    MaaToolKitTaskHandle MAA_TOOLKIT_API MaaToolKitGetTask(MaaToolKitConfigHandle config_handle, MaaSize index);

    MaaToolKitTaskHandle MAA_TOOLKIT_API MaaToolKitAddTask(MaaToolKitConfigHandle config_handle,
                                                           MaaStringView task_name, MaaToolKitTaskHandle copy_from);
    MaaBool MAA_TOOLKIT_API MaaToolKitDelTask(MaaToolKitConfigHandle config_handle, MaaStringView task_name);
    MaaBool MAA_TOOLKIT_API MaaToolKitSetTaskIndex(MaaToolKitConfigHandle config_handle, MaaStringView task_name,
                                                   MaaSize new_index);

    MaaStringView MAA_TOOLKIT_API MaaToolKitTaskName(MaaToolKitTaskHandle task_handle);
    MaaStringView MAA_TOOLKIT_API MaaToolKitGetTaskDescription(MaaToolKitTaskHandle config_handle);
    MaaBool MAA_TOOLKIT_API MaaToolKitSetTaskDescription(MaaToolKitTaskHandle config_handle,
                                                         MaaStringView new_description);
    MaaStringView MAA_TOOLKIT_API MaaToolKitGetTaskEntry(MaaToolKitTaskHandle task_handle);
    MaaBool MAA_TOOLKIT_API MaaToolKitSetTaskEntry(MaaToolKitTaskHandle task_handle, MaaStringView new_entry);
    MaaStringView MAA_TOOLKIT_API MaaToolKitGetTaskParam(MaaToolKitTaskHandle task_handle);
    MaaBool MAA_TOOLKIT_API MaaToolKitSetTaskParam(MaaToolKitTaskHandle task_handle, MaaStringView new_param);
    MaaBool MAA_TOOLKIT_API MaaToolKitGetTaskEnabled(MaaToolKitTaskHandle task_handle);
    MaaBool MAA_TOOLKIT_API MaaToolKitSetTaskEnabled(MaaToolKitTaskHandle task_handle, MaaBool new_enabled);
    MaaStringView MAA_TOOLKIT_API MaaToolKitGetTaskCustomInfo(MaaToolKitTaskHandle task_handle, MaaStringView key);
    MaaBool MAA_TOOLKIT_API MaaToolKitSetTaskCustomInfo(MaaToolKitTaskHandle task_handle, MaaStringView key,
                                                        MaaStringView value);

    MaaBool MAA_TOOLKIT_API MaaToolKitPostAllTask(MaaToolKitConfigHandle config_handle);
    MaaStatus MAA_TOOLKIT_API MaaToolKitWaitAllTask(MaaToolKitConfigHandle config_handle);
    MaaBool MAA_TOOLKIT_API MaaToolKitStopAllTask(MaaToolKitConfigHandle config_handle);

    MaaStatus MAA_TOOLKIT_API MaaToolKitTaskStatus(MaaToolKitTaskHandle task_handle);

    MaaResourceHandle MAA_TOOLKIT_API MaaToolKitGetRawResource(MaaToolKitConfigHandle config_handle);
    MaaControllerHandle MAA_TOOLKIT_API MaaToolKitGetRawController(MaaToolKitConfigHandle config_handle);
    MaaInstanceHandle MAA_TOOLKIT_API MaaToolKitGetRawInstance(MaaToolKitConfigHandle config_handle);

#ifdef __cplusplus
}
#endif
