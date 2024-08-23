/**
 * @file MaaToolkitDevice.h
 * @author
 * @brief Toolkits for working with ADB controller.
 *
 * @copyright Copyright (c) 2024
 *
 */

// IWYU pragma: private, include <MaaToolkit/MaaToolkitAPI.h>

#pragma once

#include "../MaaToolkitDef.h"

#ifdef __cplusplus
extern "C"
{
#endif

    MAA_TOOLKIT_API MaaToolkitAdbDeviceList* MaaToolkitAdbDeviceCreateList();
    MAA_TOOLKIT_API void MaaToolkitAdbDeviceDestroyList(MaaToolkitAdbDeviceList* handle);

    MAA_TOOLKIT_API MaaBool MaaToolkitAdbDeviceFind(/* out */ MaaToolkitAdbDeviceList* buffer);
    MAA_TOOLKIT_API MaaBool MaaToolkitAdbDeviceFindSpecified(const char* adb_path, /* out */ MaaToolkitAdbDeviceList* buffer);

    MAA_TOOLKIT_API MaaSize MaaToolkitAdbDeviceListSize(MaaToolkitAdbDeviceList* list);
    MAA_TOOLKIT_API MaaToolkitAdbDevice* MaaToolkitAdbDeviceListAt(MaaToolkitAdbDeviceList* list, MaaSize index);

    MAA_TOOLKIT_API const char* MaaToolkitAdbDeviceGetName(MaaToolkitAdbDevice* device);
    MAA_TOOLKIT_API const char* MaaToolkitAdbDeviceGetAdbPath(MaaToolkitAdbDevice* device);
    MAA_TOOLKIT_API const char* MaaToolkitAdbDeviceGetAddress(MaaToolkitAdbDevice* device);
    MAA_TOOLKIT_API MaaAdbScreencapMethod MaaToolkitAdbDeviceGetScreencapMethods(MaaToolkitAdbDevice* device);
    MAA_TOOLKIT_API MaaAdbInputMethod MaaToolkitAdbDeviceGetInputMethods(MaaToolkitAdbDevice* device);
    MAA_TOOLKIT_API const char* MaaToolkitAdbDeviceGetConfig(MaaToolkitAdbDevice* device);

#ifdef __cplusplus
}
#endif
