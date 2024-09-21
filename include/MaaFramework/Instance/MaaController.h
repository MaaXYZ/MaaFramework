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
        const char* agent_path,
        MaaNotificationCallback notify,
        void* notify_trans_arg);

    MAA_FRAMEWORK_API MaaController* MaaWin32ControllerCreate(
        void* hWnd,
        MaaWin32ScreencapMethod screencap_method,
        MaaWin32InputMethod input_method,
        MaaNotificationCallback notify,
        void* notify_trans_arg);

    MAA_FRAMEWORK_API MaaController* MaaCustomControllerCreate(
        MaaCustomControllerCallbacks* controller,
        void* controller_arg,
        MaaNotificationCallback notify,
        void* notify_trans_arg);

    MAA_FRAMEWORK_API MaaController* MaaDbgControllerCreate(
        const char* read_path,
        const char* write_path,
        MaaDbgControllerType type,
        const char* config,
        MaaNotificationCallback notify,
        void* notify_trans_arg);

    MAA_FRAMEWORK_API void MaaControllerDestroy(MaaController* ctrl);

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

    MAA_FRAMEWORK_API MaaCtrlId MaaControllerPostPressKey(MaaController* ctrl, int32_t keycode);

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

    MAA_FRAMEWORK_API MaaCtrlId MaaControllerPostScreencap(MaaController* ctrl);

    MAA_FRAMEWORK_API MaaStatus MaaControllerStatus(const MaaController* ctrl, MaaCtrlId id);

    MAA_FRAMEWORK_API MaaStatus MaaControllerWait(const MaaController* ctrl, MaaCtrlId id);

    MAA_FRAMEWORK_API MaaBool MaaControllerConnected(const MaaController* ctrl);

    MAA_FRAMEWORK_API MaaBool MaaControllerCachedImage(const MaaController* ctrl, /* out */ MaaImageBuffer* buffer);

    MAA_FRAMEWORK_API MaaBool MaaControllerGetUuid(MaaController* ctrl, /* out */ MaaStringBuffer* buffer);

#ifdef __cplusplus
}
#endif
