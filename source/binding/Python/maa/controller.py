from ctypes import c_int32
from abc import ABC
from typing import Any, Dict, Optional, Union
from pathlib import Path
import json

from .custom_controller import CustomControllerAgent
from .callback_agent import Callback, CallbackAgent
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
        Async connect to the controller.

        :return: True if the connection was successful, False otherwise.
        """
        await self.post_connection().wait()

    def post_connection(self) -> Future:
        """
        Post a connection to the controller. (connect in backgroud)

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

    def _status(self, maaid: int) -> MaaStatus:
        return Library.framework.MaaControllerStatus(self._handle, maaid)

    _api_properties_initialized: bool = False

    # TODO: 进一步优化 set_option？
    def set_option(self, key: MaaCtrlOptionEnum, value: MaaOptionValue) -> bool:
        if (
            key == MaaCtrlOptionEnum.ScreenshotTargetLongSide
            or key == MaaCtrlOptionEnum.ScreenshotTargetShortSide
        ):
            size = ctypes.sizeof(ctypes.c_int32)
        elif (
            key == MaaCtrlOptionEnum.DefaultAppPackage
            or key == MaaCtrlOptionEnum.DefaultAppPackageEntry
        ):
            size = ctypes.sizeof(ctypes.c_bool)
        elif key == MaaCtrlOptionEnum.Recording:
            size = ctypes.sizeof(ctypes.c_bool)
        elif key == MaaCtrlOptionEnum.Invalid:
            size = 0
        else:
            raise ValueError(f"Unsupported option: {key}")

        return bool(
            Library.framework.MaaControllerSetOption(self._handle, key, ctypes.pointer(value), size)
        )

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


class AdbController(Controller):
    def __init__(
        self,
        adb_path: Union[str, Path],
        address: str,
        type: MaaAdbControllerType = (
            MaaAdbControllerTypeEnum.Input_Preset_AutoDetect
            | MaaAdbControllerTypeEnum.Screencap_FastestWay
        ),
        config: Dict[str, Any] = {},
        agent_path: Union[str, Path] = "share/MaaAgentBinary",
        callback: Optional[Callback] = None,
        callback_arg: Any = None,
    ):
        """
        ADB controller.

        :param adb_path: The path to the ADB executable.
        :param address: The address of the device.
        :param type: The type of the controller.
        :param config: The configuration of the controller.
        :param callback: The callback function.
        :param callback_arg: The callback argument.
        """

        super().__init__()
        self._set_adb_api_properties()

        self._callback_agent = CallbackAgent(callback, callback_arg)
        self._handle = Library.framework.MaaAdbControllerCreateV2(
            str(adb_path).encode("utf-8"),
            address.encode("utf-8"),
            type,
            json.dumps(config).encode("utf-8"),
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
        type: MaaDbgControllerType = MaaDbgControllerTypeEnum.CarouselImage,
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
            type,
            json.dumps(config).encode("utf-8"),
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
        type: MaaWin32ControllerType = (
            MaaWin32ControllerTypeEnum.Key_SendMessage
            | MaaWin32ControllerTypeEnum.Touch_SendMessage
            | MaaWin32ControllerTypeEnum.Screencap_DXGI_DesktopDup
        ),
        callback: Optional[Callback] = None,
        callback_arg: Any = None,
    ):
        super().__init__()
        self._set_dbg_api_properties()

        self._callback_agent = CallbackAgent(callback, callback_arg)
        self._handle = Library.framework.MaaWin32ControllerCreate(
            hWnd,
            type,
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
            json.dumps(config).encode("utf-8"),
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
