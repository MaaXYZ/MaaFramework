/**
 * @file MaaController.h
 * @author
 * @brief The controller interface. See \ref MaaControllerPostRequest for details on how to post
 * requests to the controller.
 *
 * @copyright Copyright (c) 2024
 *
 */

// IWYU pragma: private, include <MaaFramework/MaaAPI.h>

#pragma once

#include "../MaaDef.h"
#include "../MaaPort.h"

#ifdef __cplusplus
extern "C"
{
#endif
    /**
     * @brief Create a ADB controller instance.
     *
     * @param adb_path The path of ADB executable.
     * @param address The ADB serial of the target device.
     * @param screencap_methods Use bitwise OR to set the method you need, MaaFramework will test their speed and use the fastest one.
     * @param input_methods Use bitwise OR to set the method you need, MaaFramework will select the available ones according to priority.
     * @param config The config of the ADB controller.
     * @param agent_path The path of the agent executable.
     * @param callback The callback function. See ::MaaAPICallback.
     * @param callback_arg The callback arg that will be passed to the callback function.
     * @return MaaControllerHandle The handle of the created controller instance.
     */
    MAA_FRAMEWORK_API MaaControllerHandle MaaAdbControllerCreate(
        MaaStringView adb_path,
        MaaStringView address,
        MaaAdbScreencapMethod screencap_methods,
        MaaAdbInputMethod input_methods,
        MaaStringView config,
        MaaStringView agent_path,
        MaaControllerCallback callback,
        MaaCallbackTransparentArg callback_arg);

    /**
     * @brief Create a win32 controller instance.
     *
     * @param hWnd The win32 window handle to control. This can be retrieved by helpers provided in
     * MaaToolkitWin32Window.h.
     * @param screencap_methods Use bitwise OR to set the method you need, MaaFramework will test their speed and use the fastest one.
     * @param input_methods Use bitwise OR to set the method you need, MaaFramework will select the available ones according to priority.
     * @param callback The callback function. See ::MaaAPICallback.
     * @param callback_arg The callback arg that will be passed to the callback function.
     * @return MaaControllerHandle The handle of the created controller instance.
     */
    MAA_FRAMEWORK_API MaaControllerHandle MaaWin32ControllerCreate(
        MaaWin32Hwnd hWnd,
        MaaWin32ScreencapMethod screencap_methods,
        MaaWin32InputMethod input_methods,
        MaaControllerCallback callback,
        MaaCallbackTransparentArg callback_arg);

    /**
     * @brief Create a custom controller instance.
     *
     * @param handle The handle to your instance of custom controller. See MaaCustomControllerAPI.
     * @param handle_arg The arg that will be passed to the custom controller API.
     * @param callback The callback function. See ::MaaAPICallback.
     * @param callback_arg The callback arg that will be passed to the callback function.
     * @return MaaControllerHandle The handle of the created controller instance.
     */
    MAA_FRAMEWORK_API MaaControllerHandle MaaCustomControllerCreate(
        MaaCustomControllerHandle handle,
        MaaTransparentArg handle_arg,
        MaaControllerCallback callback,
        MaaCallbackTransparentArg callback_arg);

    MAA_FRAMEWORK_API MaaControllerHandle MaaThriftControllerCreate(
        MaaThriftControllerType type,
        MaaStringView host,
        int32_t port,
        MaaStringView config,
        MaaControllerCallback callback,
        MaaCallbackTransparentArg callback_arg);

    MAA_FRAMEWORK_API MaaControllerHandle MaaDbgControllerCreate(
        MaaStringView read_path,
        MaaStringView write_path,
        MaaDbgControllerType type,
        MaaStringView config,
        MaaControllerCallback callback,
        MaaCallbackTransparentArg callback_arg);

    /**
     * @brief Free the controller instance.
     *
     * @param ctrl
     */
    MAA_FRAMEWORK_API void MaaControllerDestroy(MaaControllerHandle ctrl);

    /**
     * @brief Set options for a given controller instance.
     *
     * This function requires a given set of option keys and value types, otherwise this will fail.
     * See #MaaCtrlOptionEnum for details.
     *
     * @param ctrl The handle of the controller instance to set options for.
     * @param key The option key.
     * @param[in] value The option value.
     * @param val_size The size of the option value.
     * @return MaaBool Whether the option is set successfully.
     */
    MAA_FRAMEWORK_API MaaBool MaaControllerSetOption(
        MaaControllerHandle ctrl,
        MaaCtrlOption key,
        MaaOptionValue value /**< Maybe a byte array */,
        MaaOptionValueSize val_size);

    /**
     * @defgroup MaaControllerPostRequest Controller Requests
     *
     * The following functions post their corresponding requests to the controller and return the id
     * of the request.
     * @{
     */

    MAA_FRAMEWORK_API MaaCtrlId MaaControllerPostConnection(MaaControllerHandle ctrl);

    MAA_FRAMEWORK_API MaaCtrlId
        MaaControllerPostClick(MaaControllerHandle ctrl, int32_t x, int32_t y);

    MAA_FRAMEWORK_API MaaCtrlId MaaControllerPostSwipe(
        MaaControllerHandle ctrl,
        int32_t x1,
        int32_t y1,
        int32_t x2,
        int32_t y2,
        int32_t duration);

    MAA_FRAMEWORK_API MaaCtrlId
        MaaControllerPostPressKey(MaaControllerHandle ctrl, int32_t keycode);

    MAA_FRAMEWORK_API MaaCtrlId
        MaaControllerPostInputText(MaaControllerHandle ctrl, MaaStringView text);

    MAA_FRAMEWORK_API MaaCtrlId
        MaaControllerPostStartApp(MaaControllerHandle ctrl, MaaStringView intent);

    MAA_FRAMEWORK_API MaaCtrlId
        MaaControllerPostStopApp(MaaControllerHandle ctrl, MaaStringView intent);

    MAA_FRAMEWORK_API MaaCtrlId MaaControllerPostTouchDown(
        MaaControllerHandle ctrl,
        int32_t contact,
        int32_t x,
        int32_t y,
        int32_t pressure);
    MAA_FRAMEWORK_API MaaCtrlId MaaControllerPostTouchMove(
        MaaControllerHandle ctrl,
        int32_t contact,
        int32_t x,
        int32_t y,
        int32_t pressure);

    MAA_FRAMEWORK_API MaaCtrlId MaaControllerPostTouchUp(MaaControllerHandle ctrl, int32_t contact);

    MAA_FRAMEWORK_API MaaCtrlId MaaControllerPostScreencap(MaaControllerHandle ctrl);

    /** @} */

    /**
     * @brief Get the status of a request identified by the given id.
     *
     * @param ctrl
     * @param id
     * @return MaaStatus The status of the request.
     */
    MAA_FRAMEWORK_API MaaStatus MaaControllerStatus(MaaControllerHandle ctrl, MaaCtrlId id);

    /**
     * @brief Check if the controller is connected.
     *
     * @param ctrl
     * @return MaaBool
     */
    MAA_FRAMEWORK_API MaaBool MaaControllerConnected(MaaControllerHandle ctrl);

    /**
     * @brief Get the image buffer of the last screencap request.
     *
     * @param ctrl
     * @param buffer The buffer that the image data will be stored in.
     *
     * @return MaaBool Whether the image buffer is retrieved successfully.
     */
    MAA_FRAMEWORK_API MaaBool
        MaaControllerGetImage(MaaControllerHandle ctrl, /* out */ MaaImageBufferHandle buffer);

    /**
     * @brief Get the UUID of the controller.
     *
     * @param ctrl
     * @param buffer The buffer that the UUID will be stored in.
     *
     * @return MaaBool Whether the UUID is retrieved successfully.
     */
    MAA_FRAMEWORK_API MaaBool
        MaaControllerGetUUID(MaaControllerHandle ctrl, /* out */ MaaStringBufferHandle buffer);

#ifdef __cplusplus
}
#endif
