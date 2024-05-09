import json
from abc import ABC
from ctypes import c_int32
import os
from pathlib import Path
from typing import Any, Dict, Optional, Union

from .buffer import ImageBuffer
from .callback_agent import Callback, CallbackAgent
from .custom_controller import CustomControllerAgent
from .define import *
from .future import Future
from .library import Library

__all__ = [
    "AdbController",
    "DbgController",
    "Win32Controller",
    "ThriftController",
    "CustomController",
]


class Controller(ABC):
    _handle: MaaControllerHandle
    _callback_agent: CallbackAgent

    def __init__(self, callback: Optional[Callback] = None, callback_arg: Any = None):
        """
        Base class for all controllers.
        """

        if not Library.initialized:
            raise RuntimeError(
                "Library not initialized, please call `library.open()` first."
            )

        self._set_api_properties()

        self._callback_agent = CallbackAgent(callback, callback_arg)

    def __del__(self):
        """
        Destructor for the controller.
        """
        if self._handle:
            Library.framework.MaaControllerDestroy(self._handle)
            self._handle = None

    async def connect(self) -> bool:
        """
        Async connect.

        :return: True if the connection was successful, False otherwise.
        """
        return await self.post_connection().wait()

    def post_connection(self) -> Future:
        """
        Post a connection. (connect in backgroud)

        :return: The connection ID.
        """

        maaid = Library.framework.MaaControllerPostConnection(self._handle)
        return Future(maaid, self._status)

    @property
    def connected(self) -> bool:
        """
        Check if the controller is connected.

        :return: True if the controller is connected, False otherwise.
        """

        return bool(Library.framework.MaaControllerConnected(self._handle))

    async def screencap(self, capture: bool = True) -> Optional[numpy.ndarray]:
        """
        Async capture the screenshot.

        :param capture: Whether to capture the screen, if False, the last screenshot will be returned.
        :return: image
        """
        if capture:
            captured = await self.post_screencap().wait()
            if not captured:
                return None

        image_buffer = ImageBuffer()
        ret = Library.framework.MaaControllerGetImage(
            self._handle, image_buffer.c_handle
        )
        if not ret:
            return None
        return image_buffer.get()

    def post_screencap(self) -> Future:
        """
        Post a screencap. (get screencap in backgroud)

        :return: The screencap ID.
        """

        maaid = Library.framework.MaaControllerPostScreencap(self._handle)
        return Future(maaid, self._status)

    async def click(self, x: int, y: int) -> bool:
        """
        Async click on the controller.

        :param x: The x coordinate.
        :param y: The y coordinate.
        :return: True if the click was successful, False otherwise.
        """

        return await self.post_click(x, y).wait()

    def post_click(self, x: int, y: int) -> Future:
        """
        Post a click. (click in backgroud)

        :param x: The x coordinate.
        :param y: The y coordinate.
        :return: The click ID.
        """
        maaid = Library.framework.MaaControllerPostClick(self._handle, x, y)
        return Future(maaid, self._status)

    def set_screenshot_target_long_side(self, long_side: int) -> "Controller":
        """
        Set the screenshot target long side.

        :param long_side: The long side of the screenshot.
        """

        cint = ctypes.c_int32(long_side)
        Library.framework.MaaControllerSetOption(
            self._handle,
            MaaCtrlOptionEnum.ScreenshotTargetLongSide,
            ctypes.pointer(cint),
            ctypes.sizeof(ctypes.c_int32),
        )
        return self

    def set_screenshot_target_short_side(self, short_side: int) -> "Controller":
        """
        Set the screenshot target short side.

        :param short_side: The short side of the screenshot.
        """
        cint = ctypes.c_int32(short_side)
        Library.framework.MaaControllerSetOption(
            self._handle,
            MaaCtrlOptionEnum.ScreenshotTargetShortSide,
            ctypes.pointer(cint),
            ctypes.sizeof(ctypes.c_int32),
        )
        return self

    def _status(self, maaid: int) -> MaaStatus:
        return Library.framework.MaaControllerStatus(self._handle, maaid)

    _api_properties_initialized: bool = False

    @staticmethod
    def _set_api_properties():
        """
        Set the API properties for the controller.
        """
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

        Library.framework.MaaControllerStatus.restype = MaaStatus
        Library.framework.MaaControllerStatus.argtypes = [
            MaaControllerHandle,
            MaaCtrlId,
        ]

        Library.framework.MaaControllerConnected.restype = MaaBool
        Library.framework.MaaControllerConnected.argtypes = [MaaControllerHandle]

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
            MaaStringView,
        ]

        Library.framework.MaaControllerPostScreencap.restype = MaaCtrlId
        Library.framework.MaaControllerPostScreencap.argtypes = [
            MaaControllerHandle,
        ]

        Library.framework.MaaControllerGetImage.restype = MaaBool
        Library.framework.MaaControllerGetImage.argtypes = [
            MaaControllerHandle,
            MaaImageBufferHandle,
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
        touch_type: MaaAdbControllerType = MaaAdbControllerTypeEnum.Touch_AutoDetect,
        key_type: MaaAdbControllerType = MaaAdbControllerTypeEnum.Key_AutoDetect,
        screencap_type: MaaAdbControllerType = MaaAdbControllerTypeEnum.Screencap_FastestLosslessWay,
        config: Dict[str, Any] = {},
        agent_path: Union[str, Path] = AGENT_BINARY_PATH,
        callback: Optional[Callback] = None,
        callback_arg: Any = None,
    ):
        """
        ADB controller.

        :param adb_path: The path to the ADB executable.
        :param address: The address of the device.
        :param touch_type: The touch type.
        :param key_type: The key type.
        :param screencap_type: The screencap type.
        :param config: The configuration.
        :param callback: The callback function.
        :param callback_arg: The callback argument.
        """

        super().__init__()
        self._set_adb_api_properties()

        self._callback_agent = CallbackAgent(callback, callback_arg)
        self._handle = Library.framework.MaaAdbControllerCreateV2(
            str(adb_path).encode("utf-8"),
            address.encode("utf-8"),
            touch_type | key_type | screencap_type,
            json.dumps(config, ensure_ascii=False).encode("utf-8"),
            str(agent_path).encode("utf-8"),
            self._callback_agent.c_callback,
            self._callback_agent.c_callback_arg,
        )

        if not self._handle:
            raise RuntimeError("Failed to create ADB controller.")

    def _set_adb_api_properties(self):
        """
        Set the API properties for the ADB controller.
        """

        Library.framework.MaaAdbControllerCreateV2.restype = MaaControllerHandle
        Library.framework.MaaAdbControllerCreateV2.argtypes = [
            MaaStringView,
            MaaStringView,
            MaaAdbControllerType,
            MaaStringView,
            MaaStringView,
            MaaControllerCallback,
            MaaCallbackTransparentArg,
        ]


class DbgController(Controller):
    def __init__(
        self,
        read_path: Union[str, Path],
        write_path: Union[str, Path] = "",
        touch_type: MaaDbgControllerType = MaaDbgControllerTypeEnum.Invalid,
        key_type: MaaDbgControllerType = MaaDbgControllerTypeEnum.Invalid,
        screencap_type: MaaDbgControllerType = MaaDbgControllerTypeEnum.CarouselImage,
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
            touch_type | key_type | screencap_type,
            json.dumps(config, ensure_ascii=False).encode("utf-8"),
            self._callback_agent.c_callback,
            self._callback_agent.c_callback_arg,
        )

        if not self._handle:
            raise RuntimeError("Failed to create DBG controller.")

    def _set_dbg_api_properties(self):
        """
        Set the API properties for the DBG controller.
        """

        Library.framework.MaaDbgControllerCreate.restype = MaaControllerHandle
        Library.framework.MaaDbgControllerCreate.argtypes = [
            MaaStringView,
            MaaStringView,
            MaaDbgControllerType,
            MaaStringView,
            MaaControllerCallback,
            MaaCallbackTransparentArg,
        ]


class Win32Controller(Controller):
    def __init__(
        self,
        hWnd: MaaWin32Hwnd,
        touch_type: MaaWin32ControllerType = MaaWin32ControllerTypeEnum.Touch_Seize,
        key_type: MaaWin32ControllerType = MaaWin32ControllerTypeEnum.Key_Seize,
        screencap_type: MaaWin32ControllerType = MaaWin32ControllerTypeEnum.Screencap_DXGI_DesktopDup,
        callback: Optional[Callback] = None,
        callback_arg: Any = None,
    ):
        super().__init__()
        self._set_dbg_api_properties()

        self._callback_agent = CallbackAgent(callback, callback_arg)
        self._handle = Library.framework.MaaWin32ControllerCreate(
            hWnd,
            touch_type | key_type | screencap_type,
            self._callback_agent.c_callback,
            self._callback_agent.c_callback_arg,
        )

        if not self._handle:
            raise RuntimeError("Failed to create Win32 controller.")

    def _set_dbg_api_properties(self):
        """
        Set the API properties for the Win32 controller.
        """

        Library.framework.MaaWin32ControllerCreate.restype = MaaControllerHandle
        Library.framework.MaaWin32ControllerCreate.argtypes = [
            MaaWin32Hwnd,
            MaaWin32ControllerType,
            MaaControllerCallback,
            MaaCallbackTransparentArg,
        ]


class ThriftController(Controller):
    def __init__(
        self,
        type: MaaThriftControllerType,
        host: str,
        port: int,
        config: Dict[str, Any],
        callback: Optional[Callback] = None,
        callback_arg: Any = None,
    ):
        super().__init__()
        self._set_thrift_api_properties()

        self._callback_agent = CallbackAgent(callback, callback_arg)
        self._handle = Library.framework.MaaThriftControllerCreate(
            type,
            host.encode("utf-8"),
            port,
            json.dumps(config, ensure_ascii=False).encode("utf-8"),
            self._callback_agent.c_callback,
            self._callback_agent.c_callback_arg,
        )

        if not self._handle:
            raise RuntimeError("Failed to create Thrift controller.")

    def _set_thrift_api_properties(self):
        """
        Set the API properties for the Thrift controller.
        """

        Library.framework.MaaThriftControllerCreate.restype = MaaControllerHandle
        Library.framework.MaaThriftControllerCreate.argtypes = [
            MaaThriftControllerType,
            MaaStringView,
            c_int32,
            MaaStringView,
            MaaControllerCallback,
            MaaCallbackTransparentArg,
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
            MaaControllerCallback,
            MaaCallbackTransparentArg,
        )

        if not self._handle:
            raise RuntimeError("Failed to create Custom controller.")

    def _set_custom_api_properties(self):
        """
        Set the API properties for the Custom controller.
        """

        Library.framework.MaaCustomControllerCreate.restype = MaaControllerHandle
        Library.framework.MaaCustomControllerCreate.argtypes = [
            MaaCustomActionHandle,
            MaaTransparentArg,
            MaaControllerCallback,
            MaaCallbackTransparentArg,
        ]
