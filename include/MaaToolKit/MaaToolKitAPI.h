#pragma once

#include "MaaToolKitDef.h"

#ifdef __cplusplus
extern "C"
{
#endif

    MaaBool MAA_TOOLKIT_API MaaToolKitInit();
    MaaBool MAA_TOOLKIT_API MaaToolKitUninit();

    MaaBool MAA_TOOLKIT_API MaaToolKitStartWebServer(MaaString ip, MaaPort port);
    MaaBool MAA_TOOLKIT_API MaaToolKitStopWebServer();

    MaaToolKitConfigHandle MAA_TOOLKIT_API MaaToolKitGetCurrentConfig();
    MaaSize MAA_TOOLKIT_API MaaToolKitConfigSize();
    MaaToolKitConfigHandle MAA_TOOLKIT_API MaaToolKitGetConfig(MaaSize index);

    MaaBool MAA_TOOLKIT_API MaaToolKitAddConfig(MaaString config_name, MaaString copy_from);
    MaaBool MAA_TOOLKIT_API MaaToolKitDelConfig(MaaString config_name);

    MaaSize MAA_TOOLKIT_API MaaToolKitTaskSize();
    MaaString MAA_TOOLKIT_API MaaToolKitGetTask(MaaToolKitConfigHandle config_handle, MaaSize index);

    MaaBool MAA_TOOLKIT_API MaaToolKitAddOrUpdateTask(MaaToolKitConfigHandle config_handle, MaaString task_name,
                                                      MaaString task_type, MaaJsonString task_param);
    MaaBool MAA_TOOLKIT_API MaaToolKitDelTask(MaaToolKitConfigHandle config_handle, MaaString task_name);

    MaaBool MAA_TOOLKIT_API MaaToolKitPostAllTask(MaaToolKitConfigHandle config_handle);
    MaaBool MAA_TOOLKIT_API MaaToolKitPostTask(MaaToolKitConfigHandle config_handle, MaaString task_name);

    MaaBool MAA_TOOLKIT_API MaaToolKitStopTask();

#ifdef __cplusplus
}
#endif
