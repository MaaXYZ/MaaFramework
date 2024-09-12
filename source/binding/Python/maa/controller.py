import json
from abc import ABC
from ctypes import c_int32
import os
from pathlib import Path
from typing import Any, Dict, Optional, Union

from .buffer import ImageBuffer, StringBuffer
from .callback_agent import Callback, CallbackAgent
from .custom_controller import CustomControllerAgent
from .define import *
from .job import Job, JobWithResult
from .library import Library

__all__ = [
    "AdbController",
    "DbgController",
    "Win32Controller",
    "CustomController",
]


class Controller:
    _handle: MaaControllerHandle
    _callback_agent: CallbackAgent
    _own: bool = False

    def __init__(
        self,
        handle: MaaControllerHandle = None,
    ):
        if not Library.initialized:
            raise RuntimeError(
                "Library not initialized, please call `library.open()` first."
            )

        self._set_api_properties()

        if handle:
            self._handle = handle
            self._own = False
        else:
            # set handle by child class
            self._own = True

    def __del__(self):
        if self._handle and self._own:
            Library.framework.MaaControllerDestroy(self._handle)
            self._handle = None

    def post_connection(self) -> Job:
        ctrl_id = Library.framework.MaaControllerPostConnection(self._handle)
        return self._gen_ctrl_job(ctrl_id)

    def post_click(self, x: int, y: int) -> Job:
        ctrl_id = Library.framework.MaaControllerPostClick(self._handle, x, y)
        return self._gen_ctrl_job(ctrl_id)

    def post_swipe(self, x1: int, y1: int, x2: int, y2: int, duration: int) -> Job:
        ctrl_id = Library.framework.MaaControllerPostSwipe(
            self._handle, x1, y1, x2, y2, duration
        )
        return self._gen_ctrl_job(ctrl_id)

    def post_press_key(self, key: int) -> Job:
        ctrl_id = Library.framework.MaaControllerPostPressKey(self._handle, key)
        return self._gen_ctrl_job(ctrl_id)

    def post_input_text(self, text: str) -> Job:
        ctrl_id = Library.framework.MaaControllerPostInputText(
            self._handle, text.encode("utf-8")
        )
        return self._gen_ctrl_job(ctrl_id)

    def post_start_app(self, intent: str) -> Job:
        ctrl_id = Library.framework.MaaControllerPostStartApp(
            self._handle, intent.encode("utf-8")
        )
        return self._gen_ctrl_job(ctrl_id)

    def post_stop_app(self, package: str) -> Job:
        ctrl_id = Library.framework.MaaControllerPostStopApp(
            self._handle, package.encode("utf-8")
        )
        return self._gen_ctrl_job(ctrl_id)

    def post_touch_down(
        self, x: int, y: int, contact: int = 0, pressure: int = 1
    ) -> Job:
        ctrl_id = Library.framework.MaaControllerPostTouchDown(
            self._handle, contact, x, y, pressure
        )
        return self._gen_ctrl_job(ctrl_id)

    def post_touch_move(
        self, x: int, y: int, contact: int = 0, pressure: int = 1
    ) -> Job:
        ctrl_id = Library.framework.MaaControllerPostTouchMove(
            self._handle, contact, x, y, pressure
        )
        return self._gen_ctrl_job(ctrl_id)

    def post_touch_up(self, int, contact: int = 0) -> Job:
        ctrl_id = Library.framework.MaaControllerPostTouchUp(self._handle, contact)
        return self._gen_ctrl_job(ctrl_id)

    def post_screencap(self) -> Job:
        ctrl_id = Library.framework.MaaControllerPostScreencap(self._handle)
        return JobWithResult(
            ctrl_id,
            self._status,
            self._wait,
            self.cached_image,
        )

    @property
    def cached_image(self) -> Optional[numpy.ndarray]:
        image_buffer = ImageBuffer()
        if not Library.framework.MaaControllerCachedImage(
            self._handle, image_buffer._handle
        ):
            return None
        return image_buffer.get()

    @property
    def connected(self) -> bool:
        return bool(Library.framework.MaaControllerConnected(self._handle))

    @property
    def uuid(self) -> str:
        buffer = StringBuffer()
        if not Library.framework.MaaControllerGetUuid(self._handle, buffer._handle):
            return None
        return buffer.get()

    def set_screenshot_target_long_side(self, long_side: int) -> bool:
        cint = ctypes.c_int32(long_side)
        return bool(
            Library.framework.MaaControllerSetOption(
                self._handle,
                MaaCtrlOptionEnum.ScreenshotTargetLongSide,
                ctypes.pointer(cint),
                ctypes.sizeof(ctypes.c_int32),
            )
        )

    def set_screenshot_target_short_side(self, short_side: int) -> bool:
        cint = ctypes.c_int32(short_side)
        return bool(
            Library.framework.MaaControllerSetOption(
                self._handle,
                MaaCtrlOptionEnum.ScreenshotTargetShortSide,
                ctypes.pointer(cint),
                ctypes.sizeof(ctypes.c_int32),
            )
        )

    ### private ###

    def _status(self, maaid: int) -> MaaStatus:
        return Library.framework.MaaControllerStatus(self._handle, maaid)

    def _wait(self, maaid: int) -> MaaStatus:
        return Library.framework.MaaControllerWait(self._handle, maaid)

    def _gen_ctrl_job(self, ctrlid: MaaCtrlId) -> Job:
        return Job(
            ctrlid,
            self._status,
            self._wait,
        )

    _api_properties_initialized: bool = False

    @staticmethod
    def _set_api_properties():
        if Controller._api_properties_initialized:
            return
        Controller._api_properties_initialized = True

        Library.framework.MaaControllerDestroy.restype = None
        Library.framework.MaaControllerDestroy.argtypes = [MaaControllerHandle]

        Library.framework.MaaControllerSetOption.restype = MaaBool
        Library.framework.MaaControllerSetOption.argtypes = [
            MaaControllerHandle,
            MaaCtrlOption,
            MaaOptionValue,
            MaaOptionValueSize,
        ]

        Library.framework.MaaControllerPostConnection.restype = MaaCtrlId
        Library.framework.MaaControllerPostConnection.argtypes = [MaaControllerHandle]

        Library.framework.MaaControllerPostClick.restype = MaaCtrlId
        Library.framework.MaaControllerPostClick.argtypes = [
            MaaControllerHandle,
            c_int32,
            c_int32,
        ]

        Library.framework.MaaControllerPostSwipe.restype = MaaCtrlId
        Library.framework.MaaControllerPostSwipe.argtypes = [
            MaaControllerHandle,
            c_int32,
            c_int32,
            c_int32,
            c_int32,
            c_int32,
        ]

        Library.framework.MaaControllerPostPressKey.restype = MaaCtrlId
        Library.framework.MaaControllerPostPressKey.argtypes = [
            MaaControllerHandle,
            c_int32,
        ]

        Library.framework.MaaControllerPostInputText.restype = MaaCtrlId
        Library.framework.MaaControllerPostInputText.argtypes = [
            MaaControllerHandle,
            ctypes.c_char_p,
        ]

        Library.framework.MaaControllerPostScreencap.restype = MaaCtrlId
        Library.framework.MaaControllerPostScreencap.argtypes = [
            MaaControllerHandle,
        ]

        Library.framework.MaaControllerPostStartApp.restype = MaaCtrlId
        Library.framework.MaaControllerPostStartApp.argtypes = [
            MaaControllerHandle,
            ctypes.c_char_p,
        ]

        Library.framework.MaaControllerPostStopApp.restype = MaaCtrlId
        Library.framework.MaaControllerPostStopApp.argtypes = [
            MaaControllerHandle,
            ctypes.c_char_p,
        ]

        Library.framework.MaaControllerPostTouchDown.restype = MaaCtrlId
        Library.framework.MaaControllerPostTouchDown.argtypes = [
            MaaControllerHandle,
            c_int32,
            c_int32,
            c_int32,
            c_int32,
        ]

        Library.framework.MaaControllerPostTouchMove.restype = MaaCtrlId
        Library.framework.MaaControllerPostTouchMove.argtypes = [
            MaaControllerHandle,
            c_int32,
            c_int32,
            c_int32,
            c_int32,
        ]

        Library.framework.MaaControllerPostTouchUp.restype = MaaCtrlId
        Library.framework.MaaControllerPostTouchUp.argtypes = [
            MaaControllerHandle,
            c_int32,
        ]
        Library.framework.MaaControllerStatus.restype = MaaStatus
        Library.framework.MaaControllerStatus.argtypes = [
            MaaControllerHandle,
            MaaCtrlId,
        ]

        Library.framework.MaaControllerWait.restype = MaaStatus
        Library.framework.MaaControllerWait.argtypes = [
            MaaControllerHandle,
            MaaCtrlId,
        ]

        Library.framework.MaaControllerConnected.restype = MaaBool
        Library.framework.MaaControllerConnected.argtypes = [MaaControllerHandle]

        Library.framework.MaaControllerCachedImage.restype = MaaBool
        Library.framework.MaaControllerCachedImage.argtypes = [
            MaaControllerHandle,
            MaaImageBufferHandle,
        ]

        Library.framework.MaaControllerGetUuid.restype = MaaBool
        Library.framework.MaaControllerGetUuid.argtypes = [
            MaaControllerHandle,
            MaaStringBufferHandle,
        ]


class AdbController(Controller):
    AGENT_BINARY_PATH = os.path.join(
        os.path.dirname(__file__),
        "../MaaAgentBinary",
    )

    def __init__(
        self,
        adb_path: Union[str, Path],
        address: str,
        screencap_methods: MaaAdbScreencapMethod = MaaAdbScreencapMethodEnum.Default,
        input_methods: MaaAdbInputMethod = MaaAdbInputMethodEnum.Default,
        config: Dict[str, Any] = {},
        agent_path: Union[str, Path] = AGENT_BINARY_PATH,
        callback: Optional[Callback] = None,
        callback_arg: Any = None,
    ):
        super().__init__()
        self._set_adb_api_properties()

        self._callback_agent = CallbackAgent(callback, callback_arg)
        self._handle = Library.framework.MaaAdbControllerCreate(
            str(adb_path).encode("utf-8"),
            address.encode("utf-8"),
            screencap_methods,
            input_methods,
            json.dumps(config, ensure_ascii=False).encode("utf-8"),
            str(agent_path).encode("utf-8"),
            self._callback_agent.c_callback,
            self._callback_agent.c_callback_arg,
        )

        if not self._handle:
            raise RuntimeError("Failed to create ADB controller.")

    def _set_adb_api_properties(self):

        Library.framework.MaaAdbControllerCreate.restype = MaaControllerHandle
        Library.framework.MaaAdbControllerCreate.argtypes = [
            ctypes.c_char_p,
            ctypes.c_char_p,
            MaaAdbScreencapMethod,
            MaaAdbInputMethod,
            ctypes.c_char_p,
            ctypes.c_char_p,
            MaaNotificationCallback,
            ctypes.c_void_p,
        ]


class Win32Controller(Controller):
    def __init__(
        self,
        hWnd: ctypes.c_void_p,
        screencap_method: MaaWin32ScreencapMethod = MaaWin32ScreencapMethodEnum.DXGI_DesktopDup,
        input_method: MaaWin32InputMethod = MaaWin32InputMethodEnum.Seize,
        callback: Optional[Callback] = None,
        callback_arg: Any = None,
    ):
        super().__init__()
        self._set_win32_api_properties()

        self._callback_agent = CallbackAgent(callback, callback_arg)
        self._handle = Library.framework.MaaWin32ControllerCreate(
            hWnd,
            screencap_method,
            input_method,
            self._callback_agent.c_callback,
            self._callback_agent.c_callback_arg,
        )

        if not self._handle:
            raise RuntimeError("Failed to create Win32 controller.")

    def _set_win32_api_properties(self):
        Library.framework.MaaWin32ControllerCreate.restype = MaaControllerHandle
        Library.framework.MaaWin32ControllerCreate.argtypes = [
            ctypes.c_void_p,
            MaaWin32ScreencapMethod,
            MaaWin32InputMethod,
            MaaNotificationCallback,
            ctypes.c_void_p,
        ]


class CustomContorller(Controller):
    def __init__(
        self,
        custom_controller: CustomControllerAgent,
        callback: Optional[Callback] = None,
        callback_arg: Any = None,
    ):
        super().__init__()
        self._set_custom_api_properties()

        self._callback_agent = CallbackAgent(callback, callback_arg)
        self._handle = Library.framework.MaaCustomControllerCreate(
            custom_controller.c_handle,
            custom_controller.c_arg,
            self._callback_agent.c_callback,
            self._callback_agent.c_callback_arg,
        )

        if not self._handle:
            raise RuntimeError("Failed to create Custom controller.")

    def _set_custom_api_properties(self):
        Library.framework.MaaCustomControllerCreate.restype = MaaControllerHandle
        Library.framework.MaaCustomControllerCreate.argtypes = [
            MaaCustomControllerCallbacks,
            ctypes.c_void_p,
            MaaNotificationCallback,
            ctypes.c_void_p,
        ]


class DbgController(Controller):
    def __init__(
        self,
        read_path: Union[str, Path],
        write_path: Union[str, Path],
        dbg_type: MaaDbgControllerType,
        config: Dict[str, Any] = {},
        callback: Optional[Callback] = None,
        callback_arg: Any = None,
    ):
        super().__init__()
        self._set_dbg_api_properties()

        self._callback_agent = CallbackAgent(callback, callback_arg)
        self._handle = Library.framework.MaaDbgControllerCreate(
            str(read_path).encode("utf-8"),
            str(write_path).encode("utf-8"),
            dbg_type,
            json.dumps(config, ensure_ascii=False).encode("utf-8"),
            self._callback_agent.c_callback,
            self._callback_agent.c_callback_arg,
        )

        if not self._handle:
            raise RuntimeError("Failed to create DBG controller.")

    def _set_dbg_api_properties(self):
        Library.framework.MaaDbgControllerCreate.restype = MaaControllerHandle
        Library.framework.MaaDbgControllerCreate.argtypes = [
            ctypes.c_char_p,
            ctypes.c_char_p,
            MaaDbgControllerType,
            ctypes.c_char_p,
            MaaNotificationCallback,
            ctypes.c_void_p,
        ]
