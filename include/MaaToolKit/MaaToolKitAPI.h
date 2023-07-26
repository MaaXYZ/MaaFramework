#pragma once

#include "MaaToolKitDef.h"
#include "MaaToolKitPort.h"

#ifdef __cplusplus
extern "C"
{
#endif

    MaaString MAA_TOOLKIT_API MaaFrameworkVersion();
    MaaString MAA_TOOLKIT_API MaaToolKitVersion();

    MaaBool MAA_TOOLKIT_API MaaToolKitInit();
    MaaBool MAA_TOOLKIT_API MaaToolKitUninit();

    // MaaBool MAA_TOOLKIT_API MaaToolKitStartWebServer(MaaString ip, MaaString port);
    // MaaBool MAA_TOOLKIT_API MaaToolKitStopWebServer();

    MaaToolKitConfigHandle MaaToolKitGetCurrentConfig();
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
