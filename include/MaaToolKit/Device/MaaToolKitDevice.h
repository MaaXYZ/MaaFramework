#pragma once

#include "../MaaToolKitDef.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /* For AdbController */

    MaaSize MAA_TOOLKIT_API MaaToolKitFindDevice();
    MaaSize MAA_TOOLKIT_API MaaToolKitFindDeviceWithAdb(MaaStringView adb_path);
    MaaStringView MAA_TOOLKIT_API MaaToolKitGetDeviceName(MaaSize index);
    MaaStringView MAA_TOOLKIT_API MaaToolKitGetDeviceAdbPath(MaaSize index);
    MaaStringView MAA_TOOLKIT_API MaaToolKitGetDeviceAdbSerial(MaaSize index);
    MaaAdbControllerType MAA_TOOLKIT_API MaaToolKitGetDeviceAdbControllerType(MaaSize index);
    MaaStringView MAA_TOOLKIT_API MaaToolKitGetDeviceAdbConfig(MaaSize index);

#ifdef __cplusplus
}
#endif
