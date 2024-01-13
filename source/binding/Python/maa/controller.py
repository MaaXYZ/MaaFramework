import ctypes
import json
import asyncio
from abc import ABC
from typing import Optional, Any, Dict

from .define import *
from .future import Future
from .library import Library
from .callback_agent import CallbackAgent, Callback

__all__ = ["AdbController"]


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

    def connected(self) -> bool:
        """
        Check if the controller is connected.

        :return: True if the controller is connected, False otherwise.
        """

        return bool(Library.framework.MaaControllerConnected(self._handle))

    def _status(self, maaid: int) -> ctypes.c_int32:
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


class AdbController(Controller):
    DEFAULT_CONFIG = {}

    def __init__(
        self,
        adb_path: str,
        address: str,
        controller_type: int = 66051,  # MaaAdbControllerType_Input_Preset_Maatouch | MaaAdbControllerType_Screencap_FastestWay
        config: Dict[str, Any] = DEFAULT_CONFIG,
        agent_path: str = "./MaaAgentBinary",
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
            adb_path.encode("utf-8"),
            address.encode("utf-8"),
            controller_type,
            json.dumps(config).encode("utf-8"),
            agent_path.encode("utf-8"),
            self._callback_agent.c_callback(),
            self._callback_agent.c_callback_arg(),
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
