import pathlib
from ctypes import c_int32
from typing import Optional, Union

from .define import *

class _Framework:
    # library.py
    @staticmethod
    def MaaVersion() -> MaaStringView: ...
    @staticmethod
    def MaaSetGlobalOption(
        key: MaaGlobalOption,
        value: MaaOptionValue,
        val_size: MaaOptionValueSize,
    ) -> MaaBool: ...
    @staticmethod
    def MaaQueryRecognitionDetail(
        reco_id: MaaRecoId,
        name: MaaStringBufferHandle,
        hit: ctypes.POINTER(MaaBool),
        hit_box: MaaRectHandle,
        detail_json: MaaStringBufferHandle,
        raw: MaaImageBufferHandle,
        draws: MaaImageListBufferHandle,
    ) -> MaaBool: ...
    @staticmethod
    def MaaQueryNodeDetail(
        node_id: MaaNodeId,
        name: MaaStringBufferHandle,
        reco_id: ctypes.POINTER(MaaRecoId),
        run_completed: ctypes.POINTER(MaaBool),
    ) -> MaaBool: ...
    @staticmethod
    def MaaQueryTaskDetail(
        task_id: MaaTaskId,
        entry: MaaStringBufferHandle,
        node_id_list: ctypes.POINTER(MaaRecoId),
        node_id_list_size: ctypes.POINTER(MaaSize),
    ) -> MaaBool: ...

    # buffer.py
    # StringBuffer
    @staticmethod
    def MaaCreateStringBuffer() -> MaaStringBufferHandle: ...
    @staticmethod
    def MaaDestroyStringBuffer(handle: MaaStringBufferHandle) -> None: ...
    @staticmethod
    def MaaIsStringEmpty(handle: MaaStringBufferHandle) -> MaaBool: ...
    @staticmethod
    def MaaClearString(handle: MaaStringBufferHandle) -> MaaBool: ...
    @staticmethod
    def MaaGetString(handle: MaaStringBufferHandle) -> MaaStringView: ...
    @staticmethod
    def MaaGetStringSize(handle: MaaStringBufferHandle) -> MaaSize: ...
    @staticmethod
    def MaaSetString(handle: MaaStringBufferHandle, str: MaaStringView) -> MaaBool: ...
    @staticmethod
    def MaaSetStringEx(
        handle: MaaStringBufferHandle,
        str: MaaStringView,
        size: MaaSize,
    ) -> MaaBool: ...
    # StringListBuffer
    @staticmethod
    def MaaCreateStringListBuffer() -> MaaStringListBufferHandle: ...
    @staticmethod
    def MaaDestroyStringListBuffer(handle: MaaStringListBufferHandle) -> None: ...
    @staticmethod
    def MaaIsStringListEmpty(handle: MaaStringListBufferHandle) -> MaaBool: ...
    @staticmethod
    def MaaClearStringList(handle: MaaStringListBufferHandle) -> MaaBool: ...
    @staticmethod
    def MaaGetStringListSize(handle: MaaStringListBufferHandle) -> MaaSize: ...
    @staticmethod
    def MaaGetStringListAt(
        handle: MaaStringListBufferHandle, index: MaaSize
    ) -> MaaStringView: ...
    @staticmethod
    def MaaStringListAppend(
        handle: MaaStringListBufferHandle, value: MaaStringBufferHandle
    ) -> MaaBool: ...
    @staticmethod
    def MaaStringListRemove(
        handle: MaaStringListBufferHandle, index: MaaSize
    ) -> MaaBool: ...

    # ImageBuffer
    @staticmethod
    def MaaCreateImageBuffer() -> MaaCreateImageBuffer: ...
    @staticmethod
    def MaaDestroyImageBuffer(handle: MaaImageBufferHandle) -> None: ...
    @staticmethod
    def MaaGetImageRawData(handle: MaaImageBufferHandle) -> MaaImageRawData: ...
    @staticmethod
    def MaaGetImageWidth(handle: MaaImageBufferHandle) -> c_int32: ...
    @staticmethod
    def MaaGetImageHeight(handle: MaaImageBufferHandle) -> c_int32: ...
    @staticmethod
    def MaaGetImageType(handle: MaaImageBufferHandle) -> c_int32: ...
    @staticmethod
    def MaaSetImageRawData(
        handle: MaaImageBufferHandle,
        data: MaaImageRawData,
        width: c_int32,
        height: c_int32,
        type: c_int32,
    ) -> MaaBool: ...
    @staticmethod
    def MaaIsImageEmpty(handle: MaaImageBufferHandle) -> MaaBool: ...
    @staticmethod
    def MaaClearImage(handle: MaaImageBufferHandle) -> MaaBool: ...
    # RectBuffer
    @staticmethod
    def MaaCreateRectBuffer() -> MaaRectHandle: ...
    @staticmethod
    def MaaDestroyRectBuffer(handle: MaaRectHandle) -> None: ...
    @staticmethod
    def MaaGetRectX(handle: MaaRectHandle) -> c_int32: ...
    @staticmethod
    def MaaGetRectY(handle: MaaRectHandle) -> c_int32: ...
    @staticmethod
    def MaaGetRectW(handle: MaaRectHandle) -> c_int32: ...
    @staticmethod
    def MaaGetRectH(handle: MaaRectHandle) -> c_int32: ...
    @staticmethod
    def MaaSetRect(
        handle: MaaRectHandle,
        x: c_int32,
        y: c_int32,
        w: c_int32,
        h: c_int32,
    ) -> MaaBool: ...
    # ImageListBuffer
    @staticmethod
    def MaaCreateImageListBuffer() -> MaaImageListBufferHandle: ...
    @staticmethod
    def MaaDestroyImageListBuffer(handle: MaaImageListBufferHandle) -> None: ...
    @staticmethod
    def MaaIsImageListEmpty(handle: MaaImageListBufferHandle) -> MaaBool: ...
    @staticmethod
    def MaaClearImageList(handle: MaaImageListBufferHandle) -> MaaBool: ...
    @staticmethod
    def MaaGetImageListSize(handle: MaaImageListBufferHandle) -> MaaSize: ...
    @staticmethod
    def MaaGetImageListAt(
        handle: MaaImageListBufferHandle, index: MaaSize
    ) -> MaaImageBufferHandle: ...
    @staticmethod
    def MaaImageListAppend(
        handle: MaaImageListBufferHandle, value: MaaImageBufferHandle
    ) -> MaaBool: ...
    @staticmethod
    def MaaImageListRemove(
        handle: MaaImageListBufferHandle, index: MaaSize
    ) -> MaaBool: ...

    # context.py
    @staticmethod
    def MaaSyncContextRunTask(
        sync_context: MaaSyncContextHandle,
        task_name: MaaStringView,
        param: MaaStringView,
    ) -> MaaBool: ...
    @staticmethod
    def MaaSyncContextRunRecognition(
        sync_context: MaaSyncContextHandle,
        image: MaaStringBufferHandle,
        task_name: MaaStringView,
        task_param: MaaStringView,
        out_box: MaaRectHandle,
        out_detail: MaaStringBufferHandle,
    ) -> MaaBool: ...
    @staticmethod
    def MaaSyncContextRunAction(
        sync_context: MaaSyncContextHandle,
        task_name: MaaStringView,
        task_param: MaaStringView,
        cur_box: MaaRectHandle,
        cur_rec_detail: MaaStringBufferHandle,
    ) -> MaaBool: ...
    @staticmethod
    def MaaSyncContextClick(
        sync_context: MaaSyncContextHandle,
        x: c_int32,
        y: c_int32,
    ) -> MaaBool: ...
    @staticmethod
    def MaaSyncContextSwipe(
        sync_context: MaaSyncContextHandle,
        x1: c_int32,
        y1: c_int32,
        x2: c_int32,
        y2: c_int32,
        duration: c_int32,
    ) -> MaaBool: ...
    @staticmethod
    def MaaSyncContextPressKey(
        sync_context: MaaSyncContextHandle,
        keycode: c_int32,
    ) -> MaaBool: ...
    @staticmethod
    def MaaSyncContextInputText(
        sync_context: MaaSyncContextHandle,
        text: MaaStringView,
    ) -> MaaBool: ...
    @staticmethod
    def MaaSyncContextTouchDown(
        sync_context: MaaSyncContextHandle,
        contact: c_int32,
        x: c_int32,
        y: c_int32,
        pressure: c_int32,
    ) -> MaaBool: ...
    @staticmethod
    def MaaSyncContextTouchMove(
        sync_context: MaaSyncContextHandle,
        contact: c_int32,
        x: c_int32,
        y: c_int32,
        pressure: c_int32,
    ) -> MaaBool: ...
    @staticmethod
    def MaaSyncContextTouchUp(
        sync_context: MaaSyncContextHandle,
        contact: c_int32,
    ) -> MaaBool: ...
    @staticmethod
    def MaaSyncContextScreencap(
        sync_context: MaaSyncContextHandle,
        out_image: MaaImageBufferHandle,
    ) -> MaaBool: ...
    @staticmethod
    def MaaSyncContextCachedImage(
        sync_context: MaaSyncContextHandle,
        out_image: MaaImageBufferHandle,
    ) -> MaaBool: ...

    # controller.py
    # Contorller
    @staticmethod
    def MaaControllerDestroy(ctrl: MaaControllerHandle) -> None: ...
    @staticmethod
    def MaaControllerSetOption(
        ctrl: MaaControllerHandle,
        key: MaaCtrlOption,
        value: MaaOptionValue,
        val_size: MaaOptionValueSize,
    ) -> MaaBool: ...
    @staticmethod
    def MaaControllerPostConnection(ctrl: MaaControllerHandle) -> MaaCtrlId: ...
    @staticmethod
    def MaaControllerStatus(
        ctrl: MaaControllerHandle,
        id: MaaCtrlId,
    ) -> MaaStatus: ...
    @staticmethod
    def MaaControllerConnected(ctrl: MaaControllerHandle) -> MaaBool: ...
    @staticmethod
    def MaaControllerPostClick(
        ctrl: MaaControllerHandle, x: c_int32, y: c_int32
    ) -> MaaCtrlId: ...
    @staticmethod
    def MaaControllerPostSwipe(
        ctrl: MaaControllerHandle,
        x1: c_int32,
        y1: c_int32,
        x2: c_int32,
        y2: c_int32,
        duration: c_int32,
    ) -> MaaCtrlId: ...
    @staticmethod
    def MaaControllerPostPressKey(
        ctrl: MaaControllerHandle, keycode: c_int32
    ) -> MaaCtrlId: ...
    @staticmethod
    def MaaControllerPostInputText(
        ctrl: MaaControllerHandle, text: MaaStringView
    ) -> MaaCtrlId: ...
    @staticmethod
    def MaaControllerPostScreencap(ctrl: MaaControllerHandle) -> MaaCtrlId: ...
    @staticmethod
    def MaaControllerGetImage(
        ctrl: MaaControllerHandle,
        out_image: MaaImageBufferHandle,
    ) -> MaaBool: ...
    # AdbController
    @staticmethod
    def MaaAdbControllerCreateV2(
        adb_path: MaaStringView,
        address: MaaStringView,
        type: MaaAdbControllerType,
        config: MaaStringView,
        agent_path: MaaStringView,
        callback: MaaControllerCallback,
        callback_arg: MaaCallbackTransparentArg,
    ) -> MaaControllerHandle: ...
    # DbgController
    @staticmethod
    def MaaDbgControllerCreate(
        read_path: MaaStringView,
        write_path: MaaStringView,
        type: MaaDbgControllerType,
        config: MaaStringView,
        callback: MaaControllerCallback,
        callback_arg: MaaCallbackTransparentArg,
    ) -> MaaControllerHandle: ...
    # Win32Controller
    @staticmethod
    def MaaWin32ControllerCreate(
        hWnd: MaaWin32Hwnd,
        type: MaaWin32ControllerType,
        callback: MaaControllerCallback,
        callback_arg: MaaCallbackTransparentArg,
    ) -> MaaControllerHandle: ...
    # ThriftController
    @staticmethod
    def MaaThriftControllerCreate(
        type: MaaThriftControllerType,
        host: MaaStringView,
        port: c_int32,
        config: MaaStringView,
        callback: MaaControllerCallback,
        callback_arg: MaaCallbackTransparentArg,
    ) -> MaaControllerHandle: ...
    # CustomContorller
    @staticmethod
    def MaaCustomControllerCreate(
        handle: MaaCustomControllerHandle,
        handle_arg: MaaTransparentArg,
        callback: MaaControllerCallback,
        callback_arg: MaaCallbackTransparentArg,
    ) -> MaaControllerHandle: ...

    # instance.py
    @staticmethod
    def MaaCreate(
        callback: MaaInstanceCallback,
        callback_arg: MaaCallbackTransparentArg,
    ) -> MaaInstanceHandle: ...
    @staticmethod
    def MaaDestroy(inst: MaaInstanceHandle) -> None: ...
    @staticmethod
    def MaaBindResource(
        inst: MaaInstanceHandle,
        res: MaaResourceHandle,
    ) -> MaaBool: ...
    @staticmethod
    def MaaBindController(
        inst: MaaInstanceHandle,
        ctrl: MaaControllerHandle,
    ) -> MaaBool: ...
    @staticmethod
    def MaaInited(inst: MaaInstanceHandle) -> MaaBool: ...
    @staticmethod
    def MaaPostTask(
        inst: MaaInstanceHandle,
        entry: MaaStringView,
        param: MaaStringView,
    ) -> MaaTaskId: ...
    @staticmethod
    def MaaPostRecognition(
        inst: MaaInstanceHandle,
        entry: MaaStringView,
        param: MaaStringView,
    ) -> MaaTaskId: ...
    @staticmethod
    def MaaPostAction(
        inst: MaaInstanceHandle,
        entry: MaaStringView,
        param: MaaStringView,
    ) -> MaaTaskId: ...
    @staticmethod
    def MaaSetTaskParam(
        inst: MaaInstanceHandle,
        id: MaaTaskId,
        param: MaaStringView,
    ) -> MaaBool: ...
    @staticmethod
    def MaaTaskStatus(
        inst: MaaInstanceHandle,
        id: MaaTaskId,
    ) -> MaaStatus: ...
    @staticmethod
    def MaaRunning(inst: MaaInstanceHandle) -> MaaBool: ...
    @staticmethod
    def MaaPostStop(inst: MaaInstanceHandle) -> MaaBool: ...
    @staticmethod
    def MaaRegisterCustomRecognizer(
        inst: MaaInstanceHandle,
        name: MaaStringView,
        recognizer: MaaCustomRecognizerHandle,
        recognizer_arg: MaaTransparentArg,
    ) -> MaaBool: ...
    @staticmethod
    def MaaRegisterCustomAction(
        inst: MaaInstanceHandle,
        name: MaaStringView,
        action: MaaCustomActionHandle,
        action_arg: MaaTransparentArg,
    ) -> MaaBool: ...

    # resource.py
    @staticmethod
    def MaaResourceCreate(
        callback: MaaResourceCallback,
        callback_arg: MaaCallbackTransparentArg,
    ) -> MaaResourceHandle: ...
    @staticmethod
    def MaaResourceDestroy(res: MaaResourceHandle) -> None: ...
    @staticmethod
    def MaaResourcePostPath(
        res: MaaResourceHandle,
        path: MaaStringView,
    ) -> MaaResId: ...
    @staticmethod
    def MaaResourceStatus(
        res: MaaResourceHandle,
        id: MaaResId,
    ) -> MaaStatus: ...
    @staticmethod
    def MaaResourceLoaded(res: MaaResourceHandle) -> MaaBool: ...
    @staticmethod
    def MaaResourceClear(res: MaaResourceHandle) -> MaaBool: ...

class _Toolkit:
    # toolkit.py
    @staticmethod
    def MaaToolkitInitOptionConfig(
        user_path: MaaStringView, default_json: MaaStringView
    ) -> MaaBool: ...
    @staticmethod
    def MaaToolkitPostFindDevice() -> MaaBool: ...
    @staticmethod
    def MaaToolkitIsFindDeviceCompleted() -> MaaBool: ...
    @staticmethod
    def MaaToolkitGetDeviceCount() -> MaaSize: ...
    @staticmethod
    def MaaToolkitGetDeviceName(index: MaaSize) -> MaaStringView: ...
    @staticmethod
    def MaaToolkitGetDeviceAdbPath(index: MaaSize) -> MaaStringView: ...
    @staticmethod
    def MaaToolkitGetDeviceAdbSerial(index: MaaSize) -> MaaStringView: ...
    @staticmethod
    def MaaToolkitGetDeviceAdbControllerType(
        index: MaaSize,
    ) -> MaaAdbControllerType: ...
    @staticmethod
    def MaaToolkitGetDeviceAdbConfig(index: MaaSize) -> MaaStringView: ...
    @staticmethod
    def MaaToolkitRegisterCustomRecognizerExecutor(
        handle: MaaInstanceHandle,
        recognizer_name: MaaStringView,
        exec_path: MaaStringView,
        exec_params: ctypes.POINTER(MaaStringView),
        exec_param_size: MaaSize,
    ) -> MaaBool: ...
    @staticmethod
    def MaaToolkitRegisterCustomActionExecutor(
        handle: MaaInstanceHandle,
        action_name: MaaStringView,
        exec_path: MaaStringView,
        exec_params: ctypes.POINTER(MaaStringView),
        exec_param_size: MaaSize,
    ) -> MaaBool: ...
    @staticmethod
    def MaaToolkitFindWindow(
        class_name: MaaStringView, window_name: MaaStringView
    ) -> MaaSize: ...
    @staticmethod
    def MaaToolkitSearchWindow(
        class_name: MaaStringView, window_name: MaaStringView
    ) -> MaaSize: ...
    @staticmethod
    def MaaToolkitListWindows(
        class_name: MaaStringView, window_name: MaaStringView
    ) -> MaaSize: ...
    @staticmethod
    def MaaToolkitGetWindow(index: MaaSize) -> MaaWin32Hwnd: ...
    @staticmethod
    def MaaToolkitGetWindowClassName(
        hwnd: MaaWin32Hwnd, buffer: MaaStringBufferHandle
    ) -> MaaBool: ...
    @staticmethod
    def MaaToolkitGetWindowWindowName(
        hwnd: MaaWin32Hwnd, buffer: MaaStringBufferHandle
    ) -> MaaBool: ...
    @staticmethod
    def MaaToolkitGetCursorWindow() -> MaaWin32Hwnd: ...
    @staticmethod
    def MaaToolkitGetDesktopWindow() -> MaaWin32Hwnd: ...
    @staticmethod
    def MaaToolkitGetForegroundWindow() -> MaaWin32Hwnd: ...

class Library:
    framework: _Framework
    initialized: bool
    toolkit: _Toolkit

    @classmethod
    def open(
        cls,
        path: Union[pathlib.Path, str],
        toolkit: bool = True,
    ) -> Optional[str]:
        """
        Open the library at the given path.

        :param path: The path to the library.
        :return: True if the library was successfully opened, False otherwise.
        """
        pass

    @classmethod
    def version(cls) -> str:
        """
        Get the version of the library.

        :return: The version of the library.
        """
        pass
