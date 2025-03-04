import json
from abc import abstractmethod
from ctypes import c_int32
import os
from pathlib import Path
from typing import Any, Dict, Optional, Union

from .buffer import ImageBuffer, StringBuffer
from .notification_handler import NotificationHandler
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
    _notification_handler: Optional[NotificationHandler]
    _handle: MaaControllerHandle
    _own: bool

    def __init__(
        self,
        handle: Optional[MaaControllerHandle] = None,
    ):
        self._set_api_properties()

        if handle:
            self._handle = handle
            self._own = False
        else:
            # set handle by child class
            self._own = True

    def __del__(self):
        if self._handle and self._own:
            Library.framework().MaaControllerDestroy(self._handle)

    def post_connection(self) -> Job:
        ctrl_id = Library.framework().MaaControllerPostConnection(self._handle)
        return self._gen_ctrl_job(ctrl_id)

    def post_click(self, x: int, y: int) -> Job:
        ctrl_id = Library.framework().MaaControllerPostClick(self._handle, x, y)
        return self._gen_ctrl_job(ctrl_id)

    def post_swipe(self, x1: int, y1: int, x2: int, y2: int, duration: int) -> Job:
        ctrl_id = Library.framework().MaaControllerPostSwipe(
            self._handle, x1, y1, x2, y2, duration
        )
        return self._gen_ctrl_job(ctrl_id)

    def post_press_key(self, key: int) -> Job:
        ctrl_id = Library.framework().MaaControllerPostPressKey(self._handle, key)
        return self._gen_ctrl_job(ctrl_id)

    def post_input_text(self, text: str) -> Job:
        ctrl_id = Library.framework().MaaControllerPostInputText(
            self._handle, text.encode()
        )
        return self._gen_ctrl_job(ctrl_id)

    def post_start_app(self, intent: str) -> Job:
        ctrl_id = Library.framework().MaaControllerPostStartApp(
            self._handle, intent.encode()
        )
        return self._gen_ctrl_job(ctrl_id)

    def post_stop_app(self, intent: str) -> Job:
        ctrl_id = Library.framework().MaaControllerPostStopApp(
            self._handle, intent.encode()
        )
        return self._gen_ctrl_job(ctrl_id)

    def post_touch_down(
        self, x: int, y: int, contact: int = 0, pressure: int = 1
    ) -> Job:
        ctrl_id = Library.framework().MaaControllerPostTouchDown(
            self._handle, contact, x, y, pressure
        )
        return self._gen_ctrl_job(ctrl_id)

    def post_touch_move(
        self, x: int, y: int, contact: int = 0, pressure: int = 1
    ) -> Job:
        ctrl_id = Library.framework().MaaControllerPostTouchMove(
            self._handle, contact, x, y, pressure
        )
        return self._gen_ctrl_job(ctrl_id)

    def post_touch_up(self, contact: int = 0) -> Job:
        ctrl_id = Library.framework().MaaControllerPostTouchUp(self._handle, contact)
        return self._gen_ctrl_job(ctrl_id)

    def post_screencap(self) -> JobWithResult:
        ctrl_id = Library.framework().MaaControllerPostScreencap(self._handle)
        return JobWithResult(
            ctrl_id,
            self._status,
            self._wait,
            self._get_screencap,
        )

    @property
    def cached_image(self) -> numpy.ndarray:
        image_buffer = ImageBuffer()
        if not Library.framework().MaaControllerCachedImage(
            self._handle, image_buffer._handle
        ):
            raise RuntimeError("Failed to get cached image.")
        return image_buffer.get()

    @property
    def connected(self) -> bool:
        return bool(Library.framework().MaaControllerConnected(self._handle))

    @property
    def uuid(self) -> str:
        buffer = StringBuffer()
        if not Library.framework().MaaControllerGetUuid(self._handle, buffer._handle):
            raise RuntimeError("Failed to get UUID.")
        return buffer.get()

    def set_screenshot_target_long_side(self, long_side: int) -> bool:
        cint = ctypes.c_int32(long_side)
        return bool(
            Library.framework().MaaControllerSetOption(
                self._handle,
                MaaOption(MaaCtrlOptionEnum.ScreenshotTargetLongSide),
                ctypes.pointer(cint),
                ctypes.sizeof(ctypes.c_int32),
            )
        )

    def set_screenshot_target_short_side(self, short_side: int) -> bool:
        cint = ctypes.c_int32(short_side)
        return bool(
            Library.framework().MaaControllerSetOption(
                self._handle,
                MaaOption(MaaCtrlOptionEnum.ScreenshotTargetShortSide),
                ctypes.pointer(cint),
                ctypes.sizeof(ctypes.c_int32),
            )
        )

    def set_screenshot_use_raw_size(self, enable: bool) -> bool:
        cbool = MaaBool(enable)
        return bool(
            Library.framework().MaaControllerSetOption(
                self._handle,
                MaaOption(MaaCtrlOptionEnum.ScreenshotUseRawSize),
                ctypes.pointer(cbool),
                ctypes.sizeof(MaaBool),
            )
        )

    ### private ###

    def _status(self, maaid: int) -> MaaStatus:
        return Library.framework().MaaControllerStatus(self._handle, maaid)

    def _wait(self, maaid: int) -> MaaStatus:
        return Library.framework().MaaControllerWait(self._handle, maaid)

    def _get_screencap(self, _: int) -> numpy.ndarray:
        return self.cached_image

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

        Library.framework().MaaControllerDestroy.restype = None
        Library.framework().MaaControllerDestroy.argtypes = [MaaControllerHandle]

        Library.framework().MaaControllerSetOption.restype = MaaBool
        Library.framework().MaaControllerSetOption.argtypes = [
            MaaControllerHandle,
            MaaCtrlOption,
            MaaOptionValue,
            MaaOptionValueSize,
        ]

        Library.framework().MaaControllerPostConnection.restype = MaaCtrlId
        Library.framework().MaaControllerPostConnection.argtypes = [MaaControllerHandle]

        Library.framework().MaaControllerPostClick.restype = MaaCtrlId
        Library.framework().MaaControllerPostClick.argtypes = [
            MaaControllerHandle,
            c_int32,
            c_int32,
        ]

        Library.framework().MaaControllerPostSwipe.restype = MaaCtrlId
        Library.framework().MaaControllerPostSwipe.argtypes = [
            MaaControllerHandle,
            c_int32,
            c_int32,
            c_int32,
            c_int32,
            c_int32,
        ]

        Library.framework().MaaControllerPostPressKey.restype = MaaCtrlId
        Library.framework().MaaControllerPostPressKey.argtypes = [
            MaaControllerHandle,
            c_int32,
        ]

        Library.framework().MaaControllerPostInputText.restype = MaaCtrlId
        Library.framework().MaaControllerPostInputText.argtypes = [
            MaaControllerHandle,
            ctypes.c_char_p,
        ]

        Library.framework().MaaControllerPostScreencap.restype = MaaCtrlId
        Library.framework().MaaControllerPostScreencap.argtypes = [
            MaaControllerHandle,
        ]

        Library.framework().MaaControllerPostStartApp.restype = MaaCtrlId
        Library.framework().MaaControllerPostStartApp.argtypes = [
            MaaControllerHandle,
            ctypes.c_char_p,
        ]

        Library.framework().MaaControllerPostStopApp.restype = MaaCtrlId
        Library.framework().MaaControllerPostStopApp.argtypes = [
            MaaControllerHandle,
            ctypes.c_char_p,
        ]

        Library.framework().MaaControllerPostTouchDown.restype = MaaCtrlId
        Library.framework().MaaControllerPostTouchDown.argtypes = [
            MaaControllerHandle,
            c_int32,
            c_int32,
            c_int32,
            c_int32,
        ]

        Library.framework().MaaControllerPostTouchMove.restype = MaaCtrlId
        Library.framework().MaaControllerPostTouchMove.argtypes = [
            MaaControllerHandle,
            c_int32,
            c_int32,
            c_int32,
            c_int32,
        ]

        Library.framework().MaaControllerPostTouchUp.restype = MaaCtrlId
        Library.framework().MaaControllerPostTouchUp.argtypes = [
            MaaControllerHandle,
            c_int32,
        ]
        Library.framework().MaaControllerStatus.restype = MaaStatus
        Library.framework().MaaControllerStatus.argtypes = [
            MaaControllerHandle,
            MaaCtrlId,
        ]

        Library.framework().MaaControllerWait.restype = MaaStatus
        Library.framework().MaaControllerWait.argtypes = [
            MaaControllerHandle,
            MaaCtrlId,
        ]

        Library.framework().MaaControllerConnected.restype = MaaBool
        Library.framework().MaaControllerConnected.argtypes = [MaaControllerHandle]

        Library.framework().MaaControllerCachedImage.restype = MaaBool
        Library.framework().MaaControllerCachedImage.argtypes = [
            MaaControllerHandle,
            MaaImageBufferHandle,
        ]

        Library.framework().MaaControllerGetUuid.restype = MaaBool
        Library.framework().MaaControllerGetUuid.argtypes = [
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
        screencap_methods: int = MaaAdbScreencapMethodEnum.Default,
        input_methods: int = MaaAdbInputMethodEnum.Default,
        config: Dict[str, Any] = {},
        agent_path: Union[str, Path] = AGENT_BINARY_PATH,
        notification_handler: Optional[NotificationHandler] = None,
    ):
        super().__init__()
        self._set_adb_api_properties()

        self._notification_handler = notification_handler

        self._handle = Library.framework().MaaAdbControllerCreate(
            str(adb_path).encode(),
            address.encode(),
            MaaAdbScreencapMethod(screencap_methods),
            MaaAdbInputMethod(input_methods),
            json.dumps(config, ensure_ascii=False).encode(),
            str(agent_path).encode(),
            *NotificationHandler._gen_c_param(self._notification_handler)
        )

        if not self._handle:
            raise RuntimeError("Failed to create ADB controller.")

    def _set_adb_api_properties(self):

        Library.framework().MaaAdbControllerCreate.restype = MaaControllerHandle
        Library.framework().MaaAdbControllerCreate.argtypes = [
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
        hWnd: Union[ctypes.c_void_p, int, None],
        screencap_method: int = MaaWin32ScreencapMethodEnum.DXGI_DesktopDup,
        input_method: int = MaaWin32InputMethodEnum.Seize,
        notification_handler: Optional[NotificationHandler] = None,
    ):
        super().__init__()
        self._set_win32_api_properties()

        self._notification_handler = notification_handler
        self._handle = Library.framework().MaaWin32ControllerCreate(
            hWnd,
            MaaWin32ScreencapMethod(screencap_method),
            MaaWin32InputMethod(input_method),
            *NotificationHandler._gen_c_param(self._notification_handler)
        )

        if not self._handle:
            raise RuntimeError("Failed to create Win32 controller.")

    def _set_win32_api_properties(self):
        Library.framework().MaaWin32ControllerCreate.restype = MaaControllerHandle
        Library.framework().MaaWin32ControllerCreate.argtypes = [
            ctypes.c_void_p,
            MaaWin32ScreencapMethod,
            MaaWin32InputMethod,
            MaaNotificationCallback,
            ctypes.c_void_p,
        ]


class DbgController(Controller):

    def __init__(
        self,
        read_path: Union[str, Path],
        write_path: Union[str, Path],
        dbg_type: int,
        config: Dict[str, Any] = {},
        notification_handler: Optional[NotificationHandler] = None,
    ):
        super().__init__()
        self._set_dbg_api_properties()

        self._notification_handler = notification_handler
        self._handle = Library.framework().MaaDbgControllerCreate(
            str(read_path).encode(),
            str(write_path).encode(),
            MaaDbgControllerType(dbg_type),
            json.dumps(config, ensure_ascii=False).encode(),
            *NotificationHandler._gen_c_param(self._notification_handler)
        )

        if not self._handle:
            raise RuntimeError("Failed to create DBG controller.")

    def _set_dbg_api_properties(self):
        Library.framework().MaaDbgControllerCreate.restype = MaaControllerHandle
        Library.framework().MaaDbgControllerCreate.argtypes = [
            ctypes.c_char_p,
            ctypes.c_char_p,
            MaaDbgControllerType,
            ctypes.c_char_p,
            MaaNotificationCallback,
            ctypes.c_void_p,
        ]


class CustomController(Controller):

    _callbacks: MaaCustomControllerCallbacks

    def __init__(
        self,
        notification_handler: Optional[NotificationHandler] = None,
    ):
        super().__init__()
        self._set_custom_api_properties()

        self._notification_handler = notification_handler

        self._callbacks = MaaCustomControllerCallbacks(
            CustomController._c_connect_agent,
            CustomController._c_request_uuid_agent,
            CustomController._c_start_app_agent,
            CustomController._c_stop_app_agent,
            CustomController._c_screencap_agent,
            CustomController._c_click_agent,
            CustomController._c_swipe_agent,
            CustomController._c_touch_down_agent,
            CustomController._c_touch_move_agent,
            CustomController._c_touch_up_agent,
            CustomController._c_press_key_agent,
            CustomController._c_input_text_agent,
        )

        self._handle = Library.framework().MaaCustomControllerCreate(
            self.c_handle,
            self.c_arg,
            *NotificationHandler._gen_c_param(self._notification_handler)
        )

        if not self._handle:
            raise RuntimeError("Failed to create Custom controller.")

    @property
    def c_handle(self) -> ctypes.POINTER(MaaCustomControllerCallbacks):
        return ctypes.pointer(self._callbacks)

    @property
    def c_arg(self) -> ctypes.c_void_p:
        return ctypes.c_void_p.from_buffer(ctypes.py_object(self))

    @abstractmethod
    def connect(self) -> bool:
        raise NotImplementedError

    @abstractmethod
    def request_uuid(self) -> str:
        raise NotImplementedError

    @abstractmethod
    def start_app(self, intent: str) -> bool:
        raise NotImplementedError

    @abstractmethod
    def stop_app(self, intent: str) -> bool:
        raise NotImplementedError

    @abstractmethod
    def screencap(self) -> numpy.ndarray:
        raise NotImplementedError

    @abstractmethod
    def click(self, x: int, y: int) -> bool:
        raise NotImplementedError

    @abstractmethod
    def swipe(self, x1: int, y1: int, x2: int, y2: int, duration: int) -> bool:
        raise NotImplementedError

    @abstractmethod
    def touch_down(
        self,
        contact: int,
        x: int,
        y: int,
        pressure: int,
    ) -> bool:
        raise NotImplementedError

    @abstractmethod
    def touch_move(
        self,
        contact: int,
        x: int,
        y: int,
        pressure: int,
    ) -> bool:
        raise NotImplementedError

    @abstractmethod
    def touch_up(self, contact: int) -> bool:
        raise NotImplementedError

    @abstractmethod
    def press_key(self, keycode: int) -> bool:
        raise NotImplementedError

    @abstractmethod
    def input_text(self, text: str) -> bool:
        raise NotImplementedError

    @staticmethod
    @MaaCustomControllerCallbacks.ConnectFunc
    def _c_connect_agent(
        trans_arg: ctypes.c_void_p,
    ) -> int:
        if not trans_arg:
            return int(False)

        self: CustomController = ctypes.cast(
            trans_arg,
            ctypes.py_object,
        ).value

        return int(self.connect())

    @staticmethod
    @MaaCustomControllerCallbacks.RequestUuidFunc
    def _c_request_uuid_agent(
        trans_arg: ctypes.c_void_p,
        c_buffer: MaaStringBufferHandle,
    ) -> int:
        if not trans_arg:
            return int(False)

        self: CustomController = ctypes.cast(
            trans_arg,
            ctypes.py_object,
        ).value

        uuid = self.request_uuid()

        uuid_buffer = StringBuffer(c_buffer)
        uuid_buffer.set(uuid)
        return int(True)

    @staticmethod
    @MaaCustomControllerCallbacks.StartAppFunc
    def _c_start_app_agent(
        c_intent: ctypes.c_char_p,
        trans_arg: ctypes.c_void_p,
    ) -> int:
        if not trans_arg:
            return int(False)

        self: CustomController = ctypes.cast(
            trans_arg,
            ctypes.py_object,
        ).value

        return int(self.start_app(c_intent.decode()))

    @staticmethod
    @MaaCustomControllerCallbacks.StopAppFunc
    def _c_stop_app_agent(
        c_intent: ctypes.c_char_p,
        trans_arg: ctypes.c_void_p,
    ) -> int:
        if not trans_arg:
            return int(False)

        self: CustomController = ctypes.cast(
            trans_arg,
            ctypes.py_object,
        ).value

        return int(self.stop_app(c_intent.decode()))

    @staticmethod
    @MaaCustomControllerCallbacks.ScreencapFunc
    def _c_screencap_agent(
        trans_arg: ctypes.c_void_p,
        c_buffer: MaaStringBufferHandle,
    ) -> int:
        if not trans_arg:
            return int(False)

        self: CustomController = ctypes.cast(
            trans_arg,
            ctypes.py_object,
        ).value

        image = self.screencap()

        buffer = ImageBuffer(c_buffer)
        buffer.set(image)

        return int(True)

    @staticmethod
    @MaaCustomControllerCallbacks.ClickFunc
    def _c_click_agent(
        c_x: ctypes.c_int32,
        c_y: ctypes.c_int32,
        trans_arg: ctypes.c_void_p,
    ) -> int:
        if not trans_arg:
            return int(False)

        self: CustomController = ctypes.cast(
            trans_arg,
            ctypes.py_object,
        ).value

        return int(self.click(int(c_x), int(c_y)))

    @staticmethod
    @MaaCustomControllerCallbacks.SwipeFunc
    def _c_swipe_agent(
        c_x1: ctypes.c_int32,
        c_y1: ctypes.c_int32,
        c_x2: ctypes.c_int32,
        c_y2: ctypes.c_int32,
        c_duration: ctypes.c_int32,
        trans_arg: ctypes.c_void_p,
    ) -> int:
        if not trans_arg:
            return int(False)

        self: CustomController = ctypes.cast(
            trans_arg,
            ctypes.py_object,
        ).value

        return int(
            self.swipe(int(c_x1), int(c_y1), int(c_x2), int(c_y2), int(c_duration))
        )

    @staticmethod
    @MaaCustomControllerCallbacks.TouchDownFunc
    def _c_touch_down_agent(
        c_contact: ctypes.c_int32,
        c_x: ctypes.c_int32,
        c_y: ctypes.c_int32,
        c_pressure: ctypes.c_int32,
        trans_arg: ctypes.c_void_p,
    ) -> int:
        if not trans_arg:
            return int(False)

        self: CustomController = ctypes.cast(
            trans_arg,
            ctypes.py_object,
        ).value

        return int(self.touch_down(int(c_contact), int(c_x), int(c_y), int(c_pressure)))

    @staticmethod
    @MaaCustomControllerCallbacks.TouchMoveFunc
    def _c_touch_move_agent(
        c_contact: ctypes.c_int32,
        c_x: ctypes.c_int32,
        c_y: ctypes.c_int32,
        c_pressure: ctypes.c_int32,
        trans_arg: ctypes.c_void_p,
    ) -> int:
        if not trans_arg:
            return int(False)

        self: CustomController = ctypes.cast(
            trans_arg,
            ctypes.py_object,
        ).value

        return int(self.touch_move(int(c_contact), int(c_x), int(c_y), int(c_pressure)))

    @staticmethod
    @MaaCustomControllerCallbacks.TouchUpFunc
    def _c_touch_up_agent(
        c_contact: ctypes.c_int32,
        trans_arg: ctypes.c_void_p,
    ) -> int:
        if not trans_arg:
            return int(False)

        self: CustomController = ctypes.cast(
            trans_arg,
            ctypes.py_object,
        ).value

        return int(self.touch_up(int(c_contact)))

    @staticmethod
    @MaaCustomControllerCallbacks.PressKeyFunc
    def _c_press_key_agent(
        c_keycode: ctypes.c_int32,
        trans_arg: ctypes.c_void_p,
    ) -> int:
        if not trans_arg:
            return int(False)

        self: CustomController = ctypes.cast(
            trans_arg,
            ctypes.py_object,
        ).value

        return int(self.press_key(int(c_keycode)))

    @staticmethod
    @MaaCustomControllerCallbacks.InputTextFunc
    def _c_input_text_agent(
        c_text: ctypes.c_char_p,
        trans_arg: ctypes.c_void_p,
    ) -> int:
        if not trans_arg:
            return int(False)

        self: CustomController = ctypes.cast(
            trans_arg,
            ctypes.py_object,
        ).value

        return int(self.input_text(c_text.decode()))

    def _set_custom_api_properties(self):
        Library.framework().MaaCustomControllerCreate.restype = MaaControllerHandle
        Library.framework().MaaCustomControllerCreate.argtypes = [
            ctypes.POINTER(MaaCustomControllerCallbacks),
            ctypes.c_void_p,
            MaaNotificationCallback,
            ctypes.c_void_p,
        ]
