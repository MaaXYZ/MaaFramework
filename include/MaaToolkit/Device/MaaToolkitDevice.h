/**
 * @file MaaToolkitDevice.h
 * @author
 * @brief Toolkits for working with ADB controller.
 *
 * @copyright Copyright (c) 2024
 * 
 */

#pragma once

#include "../MaaToolkitDef.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Find ADB devices.
     *
     * \deprecated Use MaaToolkitPostFindDevice() instead.
     * 
     * @return MaaSize 
     */
    MaaSize MAA_TOOLKIT_API MaaToolkitFindDevice();

    /**
     * @brief Find ADB devices with a given ADB path.
     *
     * \deprecated Use MaaToolkitPostFindDeviceWithAdb() instead.
     *
     * @param adb_path 
     * @return MaaSize 
     */
    MaaSize MAA_TOOLKIT_API MaaToolkitFindDeviceWithAdb(MaaStringView adb_path);

    /**
     * @brief Post a request to find all ADB devices.
     *
     * @return MaaBool
     */
    MaaBool MAA_TOOLKIT_API MaaToolkitPostFindDevice();

    /**
     * @brief Post a request to find all ADB devices with a given ADB path.
     * 
     * @param adb_path 
     * @return MaaBool 
     */
    MaaBool MAA_TOOLKIT_API MaaToolkitPostFindDeviceWithAdb(MaaStringView adb_path);

    /**
     * @brief Check if the find device request is completed.
     * 
     * @return MaaBool 
     */
    MaaBool MAA_TOOLKIT_API MaaToolkitIsFindDeviceCompleted();

    /**
     * @brief Wait for the find device request to complete.
     * 
     * @return MaaSize The number of devices found.
     */
    MaaSize MAA_TOOLKIT_API MaaToolkitWaitForFindDeviceToComplete();

    /**
     * @brief Get the number of devices found.
     *
     * @return MaaSize The number of devices found.
     */
    MaaSize MAA_TOOLKIT_API MaaToolkitGetDeviceCount();

    /**
     * @brief Get the device name by index.
     *
     * @param index The 0-based index of the device. The index should not exceed the number of devices found otherwise out_of_range exception will be thrown.
     * @return MaaStringView
     */
    MaaStringView MAA_TOOLKIT_API MaaToolkitGetDeviceName(MaaSize index);

    /**
     * @brief Get the device ADB path by index.
     *
     * @param index The 0-based index of the device. The index should not exceed the number of devices found otherwise out_of_range exception will be thrown.
     * @return MaaStringView 
     */
    MaaStringView MAA_TOOLKIT_API MaaToolkitGetDeviceAdbPath(MaaSize index);

    /**
     * @brief Get the device ADB serial by index.
     *
     * @param index The 0-based index of the device. The index should not exceed the number of devices found otherwise out_of_range exception will be thrown.
     * @return MaaStringView 
     */
    MaaStringView MAA_TOOLKIT_API MaaToolkitGetDeviceAdbSerial(MaaSize index);

    /**
     * @brief Get the device ADB controller type by index.
     *
     * @param index The 0-based index of the device. The index should not exceed the number of devices found otherwise out_of_range exception will be thrown.
     * @return MaaAdbControllerType 
     */
    MaaAdbControllerType MAA_TOOLKIT_API MaaToolkitGetDeviceAdbControllerType(MaaSize index);

    /**
     * @brief Get the device ADB config by index.
     *
     * @param index The 0-based index of the device. The index should not exceed the number of devices found otherwise out_of_range exception will be thrown.
     * @return MaaStringView 
     */
    MaaStringView MAA_TOOLKIT_API MaaToolkitGetDeviceAdbConfig(MaaSize index);

#ifdef __cplusplus
}
#endif
