#pragma once

#include "../MaaDef.h"
#include "../MaaPort.h"

#ifdef __cplusplus
extern "C"
{
#endif

    // Deprecated, please use MaaAdbControllerCreateV2
    MaaControllerHandle MAA_FRAMEWORK_API MaaAdbControllerCreate( //
        MaaStringView adb_path, MaaStringView address, MaaAdbControllerType type, MaaStringView config,
        MaaControllerCallback callback, MaaCallbackTransparentArg callback_arg);

    MaaControllerHandle MAA_FRAMEWORK_API MaaWin32ControllerCreate( //
        MaaWin32Hwnd hWnd, MaaWin32ControllerType type, MaaControllerCallback callback,
        MaaCallbackTransparentArg callback_arg);

    MaaControllerHandle MAA_FRAMEWORK_API MaaAdbControllerCreateV2( //
        MaaStringView adb_path, MaaStringView address, MaaAdbControllerType type, MaaStringView config,
        MaaStringView agent_path, MaaControllerCallback callback, MaaCallbackTransparentArg callback_arg);

    MaaControllerHandle MAA_FRAMEWORK_API MaaCustomControllerCreate( //
        MaaCustomControllerHandle handle, MaaTransparentArg handle_arg, MaaControllerCallback callback,
        MaaCallbackTransparentArg callback_arg);

    MaaControllerHandle MAA_FRAMEWORK_API MaaThriftControllerCreate( //
        MaaThriftControllerType type, MaaStringView host, int32_t port, MaaStringView config,
        MaaControllerCallback callback, MaaCallbackTransparentArg callback_arg);

    MaaControllerHandle MAA_FRAMEWORK_API MaaDbgControllerCreate( //
        MaaStringView read_path, MaaStringView write_path, MaaDbgControllerType type, MaaStringView config,
        MaaControllerCallback callback, MaaCallbackTransparentArg callback_arg);

    void MAA_FRAMEWORK_API MaaControllerDestroy(MaaControllerHandle ctrl);

    MaaBool MAA_FRAMEWORK_API MaaControllerSetOption(MaaControllerHandle ctrl, MaaCtrlOption key, MaaOptionValue value,
                                                     MaaOptionValueSize val_size);

    MaaCtrlId MAA_FRAMEWORK_API MaaControllerPostConnection(MaaControllerHandle ctrl);
    MaaCtrlId MAA_FRAMEWORK_API MaaControllerPostClick(MaaControllerHandle ctrl, int32_t x, int32_t y);
    MaaCtrlId MAA_FRAMEWORK_API MaaControllerPostSwipe(MaaControllerHandle ctrl, int32_t x1, int32_t y1, int32_t x2,
                                                       int32_t y2, int32_t duration);
    MaaCtrlId MAA_FRAMEWORK_API MaaControllerPostPressKey(MaaControllerHandle ctrl, int32_t keycode);
    MaaCtrlId MAA_FRAMEWORK_API MaaControllerPostInputText(MaaControllerHandle ctrl, MaaStringView text);

    MaaCtrlId MAA_FRAMEWORK_API MaaControllerPostTouchDown(MaaControllerHandle ctrl, int32_t contact, int32_t x,
                                                           int32_t y, int32_t pressure);
    MaaCtrlId MAA_FRAMEWORK_API MaaControllerPostTouchMove(MaaControllerHandle ctrl, int32_t contact, int32_t x,
                                                           int32_t y, int32_t pressure);
    MaaCtrlId MAA_FRAMEWORK_API MaaControllerPostTouchUp(MaaControllerHandle ctrl, int32_t contact);

    MaaCtrlId MAA_FRAMEWORK_API MaaControllerPostScreencap(MaaControllerHandle ctrl);

    MaaStatus MAA_FRAMEWORK_API MaaControllerStatus(MaaControllerHandle ctrl, MaaCtrlId id);
    MaaStatus MAA_FRAMEWORK_API MaaControllerWait(MaaControllerHandle ctrl, MaaCtrlId id);
    MaaBool MAA_FRAMEWORK_API MaaControllerConnected(MaaControllerHandle ctrl);

    MaaBool MAA_FRAMEWORK_API MaaControllerGetImage(MaaControllerHandle ctrl, /* out */ MaaImageBufferHandle buffer);
    MaaBool MAA_FRAMEWORK_API MaaControllerGetUUID(MaaControllerHandle ctrl, /* out */ MaaStringBufferHandle buffer);

#ifdef __cplusplus
}
#endif
