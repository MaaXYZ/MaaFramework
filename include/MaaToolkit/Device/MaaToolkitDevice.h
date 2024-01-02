#pragma once

#include "../MaaToolkitDef.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /* For AdbController */

    // Deprecated, please use MaaToolkitPostFindDevice instead.
    MaaSize MAA_TOOLKIT_API MaaToolkitFindDevice();
    // Deprecated, please use MaaToolkitPostFindDeviceWithAdb instead.
    MaaSize MAA_TOOLKIT_API MaaToolkitFindDeviceWithAdb(MaaStringView adb_path);

    MaaBool MAA_TOOLKIT_API MaaToolkitPostFindDevice();
    MaaBool MAA_TOOLKIT_API MaaToolkitPostFindDeviceWithAdb(MaaStringView adb_path);
    MaaBool MAA_TOOLKIT_API MaaToolkitIsFindDeviceCompleted();
    MaaSize MAA_TOOLKIT_API MaaToolkitWaitForFindDeviceToComplete();

    MaaSize MAA_TOOLKIT_API MaaToolkitGetDeviceCount();
    MaaStringView MAA_TOOLKIT_API MaaToolkitGetDeviceName(MaaSize index);
    MaaStringView MAA_TOOLKIT_API MaaToolkitGetDeviceAdbPath(MaaSize index);
    MaaStringView MAA_TOOLKIT_API MaaToolkitGetDeviceAdbSerial(MaaSize index);
    MaaAdbControllerType MAA_TOOLKIT_API MaaToolkitGetDeviceAdbControllerType(MaaSize index);
    MaaStringView MAA_TOOLKIT_API MaaToolkitGetDeviceAdbConfig(MaaSize index);

#ifdef __cplusplus
}
#endif
