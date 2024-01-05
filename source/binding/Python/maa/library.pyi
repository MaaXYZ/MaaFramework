from ctypes import c_int32
import pathlib
from typing import Optional, Union

from .define import *

class _Framework:
    # library.py
    @staticmethod
    def MaaVersion() -> MaaStringView: ...

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

    # context.py
    @staticmethod
    def MaaSyncContextRunTask(
        sync_context: MaaSyncContextHandle,
        task_name: MaaStringView,
        param: MaaStringView,
    ) -> MaaBool: ...
    @staticmethod
    def MaaSyncContextRunRecognizer(
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
    def MaaSyncContextGetTaskResult(
        sync_context: MaaSyncContextHandle,
        task_name: MaaStringView,
        out_task_result: MaaStringBufferHandle,
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
    def MaaTaskAllFinished(inst: MaaInstanceHandle) -> MaaBool: ...
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

class _Toolkit:
    # toolkit.py
    @staticmethod
    def MaaToolkitInit() -> MaaBool: ...
    @staticmethod
    def MaaToolkitUninit() -> MaaBool: ...
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
        recognizer_exec_path: MaaStringView,
        recognizer_exec_param_json: MaaStringView,
    ) -> MaaBool: ...
    @staticmethod
    def MaaToolkitRegisterCustomActionExecutor(
        handle: MaaInstanceHandle,
        action_name: MaaStringView,
        action_exec_path: MaaStringView,
        action_ex: MaaStringView,
    ) -> MaaBool: ...

class Library:
    framework: _Framework
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
