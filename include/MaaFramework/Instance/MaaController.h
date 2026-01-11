/**
 * @file MaaController.h
 * @author
 * @brief The controller interface. See \ref MaaControllerPostRequest for details on how to post requests to the controller.
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

    typedef struct MaaCustomControllerCallbacks MaaCustomControllerCallbacks;

    MAA_FRAMEWORK_API MaaController* MaaAdbControllerCreate(
        const char* adb_path,
        const char* address,
        MaaAdbScreencapMethod screencap_methods,
        MaaAdbInputMethod input_methods,
        const char* config,
        const char* agent_path);

    MAA_FRAMEWORK_API MaaController* MaaWin32ControllerCreate(
        void* hWnd,
        MaaWin32ScreencapMethod screencap_method,
        MaaWin32InputMethod mouse_method,
        MaaWin32InputMethod keyboard_method);

    MAA_FRAMEWORK_API MaaController* MaaCustomControllerCreate(MaaCustomControllerCallbacks* controller, void* controller_arg);

    MAA_FRAMEWORK_API MaaController*
        MaaDbgControllerCreate(const char* read_path, const char* write_path, MaaDbgControllerType type, const char* config);

    /**
     * @brief Create a PlayCover controller for macOS.
     *
     * @param address The PlayTools service address in "host:port" format.
     * @param uuid The application bundle identifier (e.g., "com.hypergryph.arknights").
     * @return The controller handle, or nullptr on failure.
     *
     * @note This controller is designed for PlayCover on macOS.
     * @note Some features are not supported: start_app, input_text, click_key, key_down, key_up, scroll.
     * @note Only single touch is supported (contact must be 0).
     */
    MAA_FRAMEWORK_API MaaController* MaaPlayCoverControllerCreate(const char* address, const char* uuid);

    MAA_FRAMEWORK_API void MaaControllerDestroy(MaaController* ctrl);

    MAA_FRAMEWORK_API MaaSinkId MaaControllerAddSink(MaaController* ctrl, MaaEventCallback sink, void* trans_arg);

    MAA_FRAMEWORK_API void MaaControllerRemoveSink(MaaController* ctrl, MaaSinkId sink_id);

    MAA_FRAMEWORK_API void MaaControllerClearSinks(MaaController* ctrl);

    /**
     * @param[in] value
     */
    MAA_FRAMEWORK_API MaaBool MaaControllerSetOption(
        MaaController* ctrl,
        MaaCtrlOption key,
        MaaOptionValue value /**< byte array, int*, char*, bool* */,
        MaaOptionValueSize val_size);

    MAA_FRAMEWORK_API MaaCtrlId MaaControllerPostConnection(MaaController* ctrl);

    MAA_FRAMEWORK_API MaaCtrlId MaaControllerPostClick(MaaController* ctrl, int32_t x, int32_t y);

    MAA_FRAMEWORK_API MaaCtrlId
        MaaControllerPostSwipe(MaaController* ctrl, int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t duration);

    MAA_FRAMEWORK_API MaaCtrlId MaaControllerPostClickKey(MaaController* ctrl, int32_t keycode);

    MAA_FRAMEWORK_API MaaCtrlId MaaControllerPostInputText(MaaController* ctrl, const char* text);

    MAA_FRAMEWORK_API MaaCtrlId MaaControllerPostStartApp(MaaController* ctrl, const char* intent);

    MAA_FRAMEWORK_API MaaCtrlId MaaControllerPostStopApp(MaaController* ctrl, const char* intent);

    // for adb controller, contact means finger id (0 for first finger, 1 for second finger, etc)
    // for win32 controller, contact means mouse button id (0 for left, 1 for right, 2 for middle)
    MAA_FRAMEWORK_API MaaCtrlId MaaControllerPostTouchDown(MaaController* ctrl, int32_t contact, int32_t x, int32_t y, int32_t pressure);

    MAA_FRAMEWORK_API MaaCtrlId MaaControllerPostTouchMove(MaaController* ctrl, int32_t contact, int32_t x, int32_t y, int32_t pressure);

    // for adb controller, contact means finger id (0 for first finger, 1 for second finger, etc)
    // for win32 controller, contact means mouse button id (0 for left, 1 for right, 2 for middle)
    MAA_FRAMEWORK_API MaaCtrlId MaaControllerPostTouchUp(MaaController* ctrl, int32_t contact);

    MAA_FRAMEWORK_API MaaCtrlId MaaControllerPostKeyDown(MaaController* ctrl, int32_t keycode);

    MAA_FRAMEWORK_API MaaCtrlId MaaControllerPostKeyUp(MaaController* ctrl, int32_t keycode);

    MAA_FRAMEWORK_API MaaCtrlId MaaControllerPostScreencap(MaaController* ctrl);

    /**
     * @brief Post a scroll action to the controller.
     *
     * @param ctrl The controller handle.
     * @param dx The horizontal scroll delta. Positive values scroll right, negative values scroll left.
     * @param dy The vertical scroll delta. Positive values scroll up, negative values scroll down.
     * @return The control id of the scroll action.
     *
     * @note Not all controllers support scroll. If not supported, the action will fail.
     * @note The dx/dy values are sent directly as scroll increments. Using multiples of 120 (WHEEL_DELTA) is
     * recommended for best compatibility.
     */
    MAA_FRAMEWORK_API MaaCtrlId MaaControllerPostScroll(MaaController* ctrl, int32_t dx, int32_t dy);

    /**
     * @brief Post a shell command to the controller.
     *
     * @param ctrl The controller handle.
     * @param cmd The shell command to execute.
     * @param timeout Timeout in milliseconds. Default is 20000 (20 seconds).
     * @return The control id of the shell action.
     *
     * @note This is only valid for ADB controllers. If the controller is not an ADB controller, the action will fail.
     * @see MaaControllerGetShellOutput
     */
    MAA_FRAMEWORK_API MaaCtrlId MaaControllerPostShell(MaaController* ctrl, const char* cmd, int64_t timeout);

    /**
     * @brief Get the cached shell command output.
     *
     * @param ctrl The controller handle.
     * @param buffer The output buffer to store the command result.
     * @return true if the output is available, false otherwise.
     *
     * @note This returns the output from the most recent shell command execution.
     */
    MAA_FRAMEWORK_API MaaBool MaaControllerGetShellOutput(const MaaController* ctrl, /* out */ MaaStringBuffer* buffer);

    MAA_FRAMEWORK_API MaaStatus MaaControllerStatus(const MaaController* ctrl, MaaCtrlId id);

    MAA_FRAMEWORK_API MaaStatus MaaControllerWait(const MaaController* ctrl, MaaCtrlId id);

    MAA_FRAMEWORK_API MaaBool MaaControllerConnected(const MaaController* ctrl);

    MAA_FRAMEWORK_API MaaBool MaaControllerCachedImage(const MaaController* ctrl, /* out */ MaaImageBuffer* buffer);

    MAA_FRAMEWORK_API MaaBool MaaControllerGetUuid(MaaController* ctrl, /* out */ MaaStringBuffer* buffer);

    MAA_DEPRECATED MAA_FRAMEWORK_API MaaCtrlId MaaControllerPostPressKey(MaaController* ctrl, int32_t keycode);

#ifdef __cplusplus
}
#endif
