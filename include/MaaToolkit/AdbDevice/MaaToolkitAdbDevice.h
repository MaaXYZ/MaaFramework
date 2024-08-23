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

    MAA_TOOLKIT_API MaaToolkitAdbDeviceList* MaaToolkitAdbDeviceListCreate();
    MAA_TOOLKIT_API void MaaToolkitAdbDeviceListDestroy(MaaToolkitAdbDeviceList* handle);

    MAA_TOOLKIT_API MaaBool MaaToolkitAdbDeviceFind(/* out */ MaaToolkitAdbDeviceList* buffer);
    MAA_TOOLKIT_API MaaBool MaaToolkitAdbDeviceFindSpecified(const char* adb_path, /* out */ MaaToolkitAdbDeviceList* buffer);

    MAA_TOOLKIT_API MaaSize MaaToolkitAdbDeviceListSize(const MaaToolkitAdbDeviceList* list);
    MAA_TOOLKIT_API const MaaToolkitAdbDevice* MaaToolkitAdbDeviceListAt(const MaaToolkitAdbDeviceList* list, MaaSize index);

    MAA_TOOLKIT_API const char* MaaToolkitAdbDeviceGetName(const MaaToolkitAdbDevice* device);
    MAA_TOOLKIT_API const char* MaaToolkitAdbDeviceGetAdbPath(const MaaToolkitAdbDevice* device);
    MAA_TOOLKIT_API const char* MaaToolkitAdbDeviceGetAddress(const MaaToolkitAdbDevice* device);
    MAA_TOOLKIT_API MaaAdbScreencapMethod MaaToolkitAdbDeviceGetScreencapMethods(const MaaToolkitAdbDevice* device);
    MAA_TOOLKIT_API MaaAdbInputMethod MaaToolkitAdbDeviceGetInputMethods(const MaaToolkitAdbDevice* device);
    MAA_TOOLKIT_API const char* MaaToolkitAdbDeviceGetConfig(const MaaToolkitAdbDevice* device);

#ifdef __cplusplus
}
#endif
