/**
 * @file MaaController.h
 * @author
 * @brief The controller interface. See \ref MaaControllerPostRequest for details on how to post requests to the
 * controller.
 *
 * @copyright Copyright (c) 2024
 *
 */

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
     * \deprecated Use MaaAdbControllerCreateV2() instead.
     *
     * @param adb_path The path of ADB executable.
     * @param address The ADB serial of the target device.
     * @param type The type of the ADB controller. See #MaaAdbControllerTypeEnum.
     * @param config The config of the ADB controller.
     * @param callback The callback function. See ::MaaAPICallback.
     * @param callback_arg The callback arg that will be passed to the callback function.
     * @return MaaControllerHandle The handle of the created controller instance.
     */
    MaaControllerHandle MAA_FRAMEWORK_API MaaAdbControllerCreate(
        MaaStringView adb_path,
        MaaStringView address,
        MaaAdbControllerType type,
        MaaStringView config,
        MaaControllerCallback callback,
        MaaCallbackTransparentArg callback_arg);

    /**
     * @brief Create a win32 controller instance.
     *
     * @param hWnd The win32 window handle to control. This can be retrieved by helpers provided in
     * MaaToolkitWin32Window.h.
     * @param type The type of the win32 controller. See #MaaWin32ControllerTypeEnum.
     * @param callback The callback function. See ::MaaAPICallback.
     * @param callback_arg The callback arg that will be passed to the callback function.
     * @return MaaControllerHandle The handle of the created controller instance.
     */
    MaaControllerHandle MAA_FRAMEWORK_API MaaWin32ControllerCreate(
        MaaWin32Hwnd hWnd,
        MaaWin32ControllerType type,
        MaaControllerCallback callback,
        MaaCallbackTransparentArg callback_arg);

    /**
     * @brief Create a ADB controller instance.
     *
     * @param adb_path The path of ADB executable.
     * @param address The ADB serial of the target device.
     * @param type The type of the ADB controller. See #MaaAdbControllerTypeEnum.
     * @param config The config of the ADB controller.
     * @param agent_path The path of the agent executable.
     * @param callback The callback function. See ::MaaAPICallback.
     * @param callback_arg The callback arg that will be passed to the callback function.
     * @return MaaControllerHandle The handle of the created controller instance.
     */
    MaaControllerHandle MAA_FRAMEWORK_API MaaAdbControllerCreateV2(
        MaaStringView adb_path,
        MaaStringView address,
        MaaAdbControllerType type,
        MaaStringView config,
        MaaStringView agent_path,
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
    MaaControllerHandle MAA_FRAMEWORK_API MaaCustomControllerCreate(
        MaaCustomControllerHandle handle,
        MaaTransparentArg handle_arg,
        MaaControllerCallback callback,
        MaaCallbackTransparentArg callback_arg);

    MaaControllerHandle MAA_FRAMEWORK_API MaaThriftControllerCreate(
        MaaThriftControllerType type,
        MaaStringView host,
        int32_t port,
        MaaStringView config,
        MaaControllerCallback callback,
        MaaCallbackTransparentArg callback_arg);

    MaaControllerHandle MAA_FRAMEWORK_API MaaDbgControllerCreate(
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
    void MAA_FRAMEWORK_API MaaControllerDestroy(MaaControllerHandle ctrl);

    /**
     * @brief Set options for a given controller instance.
     *
     * This function requires a given set of option keys and value types, otherwise this will fail. See
     * #MaaCtrlOptionEnum for details.
     *
     * @param ctrl The handle of the controller instance to set options for.
     * @param key The option key.
     * @param value The option value.
     * @param val_size The size of the option value.
     * @return MaaBool Whether the option is set successfully.
     */
    MaaBool MAA_FRAMEWORK_API MaaControllerSetOption(
        MaaControllerHandle ctrl, MaaCtrlOption key, MaaOptionValue value, MaaOptionValueSize val_size);

    /**
     * @defgroup MaaControllerPostRequest Controller Requests
     *
     * The following functions post their corresponding requests to the controller and return the id of the request.
     * @{
     */

    MaaCtrlId MAA_FRAMEWORK_API MaaControllerPostConnection(MaaControllerHandle ctrl);

    MaaCtrlId MAA_FRAMEWORK_API MaaControllerPostClick(MaaControllerHandle ctrl, int32_t x, int32_t y);

    MaaCtrlId MAA_FRAMEWORK_API MaaControllerPostSwipe(
        MaaControllerHandle ctrl, int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t duration);

    MaaCtrlId MAA_FRAMEWORK_API MaaControllerPostPressKey(MaaControllerHandle ctrl, int32_t keycode);

    MaaCtrlId MAA_FRAMEWORK_API MaaControllerPostInputText(MaaControllerHandle ctrl, MaaStringView text);

    MaaCtrlId MAA_FRAMEWORK_API
        MaaControllerPostTouchDown(MaaControllerHandle ctrl, int32_t contact, int32_t x, int32_t y, int32_t pressure);
    MaaCtrlId MAA_FRAMEWORK_API
        MaaControllerPostTouchMove(MaaControllerHandle ctrl, int32_t contact, int32_t x, int32_t y, int32_t pressure);

    MaaCtrlId MAA_FRAMEWORK_API MaaControllerPostTouchUp(MaaControllerHandle ctrl, int32_t contact);

    MaaCtrlId MAA_FRAMEWORK_API MaaControllerPostScreencap(MaaControllerHandle ctrl);

    /** @} */

    /**
     * @brief Get the status of a request identified by the given id.
     *
     * @param ctrl
     * @param id
     * @return MaaStatus The status of the request.
     */
    MaaStatus MAA_FRAMEWORK_API MaaControllerStatus(MaaControllerHandle ctrl, MaaCtrlId id);

    /**
     * @brief Wait for the request identified by the given id to complete.
     *
     * @param ctrl
     * @param id
     * @return MaaStatus The status of the request.
     */
    MaaStatus MAA_FRAMEWORK_API MaaControllerWait(MaaControllerHandle ctrl, MaaCtrlId id);

    /**
     * @brief Check if the controller is connected.
     *
     * @param ctrl
     * @return MaaBool
     */
    MaaBool MAA_FRAMEWORK_API MaaControllerConnected(MaaControllerHandle ctrl);

    /**
     * @brief Get the image buffer of the last screencap request.
     *
     * @param ctrl
     * @param[out] buffer The buffer that the image data will be stored in.
     *
     * @return MaaBool Whether the image buffer is retrieved successfully.
     */
    MaaBool MAA_FRAMEWORK_API MaaControllerGetImage(MaaControllerHandle ctrl, MaaImageBufferHandle buffer);

    /**
     * @brief Get the UUID of the controller.
     *
     * @param ctrl
     * @param[out] buffer The buffer that the UUID will be stored in.
     *
     * @return MaaBool Whether the resolution is retrieved successfully.
     */
    MaaBool MAA_FRAMEWORK_API MaaControllerGetUUID(MaaControllerHandle ctrl, MaaStringBufferHandle buffer);

#ifdef __cplusplus
}
#endif
