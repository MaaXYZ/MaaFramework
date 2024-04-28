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

    /// \deprecated Use MaaToolkitPostFindDevice() and MaaToolkitWaitForFindDeviceToComplete()
    /// instead.
    MAA_DEPRECATED MAA_TOOLKIT_API MaaSize MaaToolkitFindDevice();

    /// \deprecated Use MaaToolkitPostFindDeviceWithAdb() and
    /// MaaToolkitWaitForFindDeviceToComplete() instead.
    MAA_DEPRECATED MAA_TOOLKIT_API MaaSize MaaToolkitFindDeviceWithAdb(MaaStringView adb_path);

    /**
     * @brief Post a request to find all ADB devices.
     *
     * @return MaaBool
     */
    MAA_TOOLKIT_API MaaBool MaaToolkitPostFindDevice();

    /**
     * @brief Post a request to find all ADB devices with a given ADB path.
     *
     * @param adb_path
     * @return MaaBool
     */
    MAA_TOOLKIT_API MaaBool MaaToolkitPostFindDeviceWithAdb(MaaStringView adb_path);

    /**
     * @brief Check if the find device request is completed.
     *
     * @return MaaBool
     */
    MAA_TOOLKIT_API MaaBool MaaToolkitIsFindDeviceCompleted();

    /**
     * @brief Wait for the find device request to complete.
     *
     * @return MaaSize The number of devices found.
     */
    MAA_TOOLKIT_API MaaSize MaaToolkitWaitForFindDeviceToComplete();

    /**
     * @brief Get the number of devices found.
     *
     * @return MaaSize The number of devices found.
     */
    MAA_TOOLKIT_API MaaSize MaaToolkitGetDeviceCount();

    /**
     * @brief Get the device name by index.
     *
     * @param index The 0-based index of the device. The index should not exceed the number of
     * devices found otherwise out_of_range exception will be thrown.
     * @return MaaStringView
     */
    MAA_TOOLKIT_API MaaStringView MaaToolkitGetDeviceName(MaaSize index);

    /**
     * @brief Get the device ADB path by index.
     *
     * @param index The 0-based index of the device. The index should not exceed the number of
     * devices found otherwise out_of_range exception will be thrown.
     * @return MaaStringView
     */
    MAA_TOOLKIT_API MaaStringView MaaToolkitGetDeviceAdbPath(MaaSize index);

    /**
     * @brief Get the device ADB serial by index.
     *
     * @param index The 0-based index of the device. The index should not exceed the number of
     * devices found otherwise out_of_range exception will be thrown.
     * @return MaaStringView
     */
    MAA_TOOLKIT_API MaaStringView MaaToolkitGetDeviceAdbSerial(MaaSize index);

    /**
     * @brief Get the device ADB controller type by index.
     *
     * @param index The 0-based index of the device. The index should not exceed the number of
     * devices found otherwise out_of_range exception will be thrown.
     * @return MaaAdbControllerType
     */
    MAA_TOOLKIT_API MaaAdbControllerType MaaToolkitGetDeviceAdbControllerType(MaaSize index);

    /**
     * @brief Get the device ADB config by index.
     *
     * @param index The 0-based index of the device. The index should not exceed the number of
     * devices found otherwise out_of_range exception will be thrown.
     * @return MaaStringView
     */
    MAA_TOOLKIT_API MaaStringView MaaToolkitGetDeviceAdbConfig(MaaSize index);

#ifdef __cplusplus
}
#endif
