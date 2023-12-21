#pragma once

#include "../MaaToolKitDef.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /* For AdbController */

    // Deprecated, please use MaaToolKitPostFindDevice instead.
    MaaSize MAA_TOOLKIT_API MaaToolKitFindDevice();
    // Deprecated, please use MaaToolKitPostFindDeviceWithAdb instead.
    MaaSize MAA_TOOLKIT_API MaaToolKitFindDeviceWithAdb(MaaStringView adb_path);

    MaaBool MAA_TOOLKIT_API MaaToolKitPostFindDevice();
    MaaBool MAA_TOOLKIT_API MaaToolKitPostFindDeviceWithAdb(MaaStringView adb_path);
    MaaBool MAA_TOOLKIT_API MaaToolKitIsFindDeviceCompleted();
    MaaSize MAA_TOOLKIT_API MaaToolKitWaitForFindDeviceToComplete();

    MaaSize MAA_TOOLKIT_API MaaToolKitGetDeviceCount();
    MaaStringView MAA_TOOLKIT_API MaaToolKitGetDeviceName(MaaSize index);
    MaaStringView MAA_TOOLKIT_API MaaToolKitGetDeviceAdbPath(MaaSize index);
    MaaStringView MAA_TOOLKIT_API MaaToolKitGetDeviceAdbSerial(MaaSize index);
    MaaAdbControllerType MAA_TOOLKIT_API MaaToolKitGetDeviceAdbControllerType(MaaSize index);
    MaaStringView MAA_TOOLKIT_API MaaToolKitGetDeviceAdbConfig(MaaSize index);

#ifdef __cplusplus
}
#endif
