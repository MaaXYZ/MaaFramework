import ctypes
import json
import asyncio
from dataclasses import dataclass
from typing import List, Union, Dict
from pathlib import Path

from .library import Library
from .define import *
from .instance import Instance


@dataclass
class AdbDevice:
    name: str
    adb_path: Path
    address: str
    controller_type: int
    config: str


class Toolkit:
    @classmethod
    def init_option(
        cls, user_path: Union[str, Path], default_config: Dict = {}
    ) -> bool:
        """
        Init the toolkit option config.
        """
        cls._set_api_properties()

        return Library.toolkit.MaaToolkitInitOptionConfig(
            str(user_path).encode("utf-8"), json.dumps(default_config).encode("utf-8")
        )

    @classmethod
    async def adb_devices(cls) -> List[AdbDevice]:
        """
        Get the adb devices.
        """
        cls._set_api_properties()

        Library.toolkit.MaaToolkitPostFindDevice()

        while not Library.toolkit.MaaToolkitIsFindDeviceCompleted():
            await asyncio.sleep(0)

        count = Library.toolkit.MaaToolkitGetDeviceCount()
        devices = []
        for i in range(count):
            name = Library.toolkit.MaaToolkitGetDeviceName(i).decode("utf-8")
            adb_path = Path(
                Library.toolkit.MaaToolkitGetDeviceAdbPath(i).decode("utf-8")
            )
            address = Library.toolkit.MaaToolkitGetDeviceAdbSerial(i).decode("utf-8")
            controller_type = int(
                Library.toolkit.MaaToolkitGetDeviceAdbControllerType(i)
            )
            config = Library.toolkit.MaaToolkitGetDeviceAdbConfig(i).decode("utf-8")

            devices.append(AdbDevice(name, adb_path, address, controller_type, config))

        return devices

    @classmethod
    def register_recognizer_exec_agent(
        cls, inst: Instance, name: str, exec_path: Union[str, Path], argv: list
    ) -> bool:
        """
        Register a recognizer exec agent.
        """
        cls._set_api_properties()

        json_argv = json.dumps(argv)
        return Library.toolkit.MaaToolkitRegisterCustomRecognizerExecutor(
            inst.c_handle,
            name.encode("utf-8"),
            str(exec_path).encode("utf-8"),
            json_argv.encode("utf-8"),
        )

    @classmethod
    def register_action_exec_agent(
        cls, inst: Instance, name: str, exec_path: Union[str, Path], argv: list
    ) -> bool:
        """
        Register a action exec agent.
        """
        cls._set_api_properties()

        json_argv = json.dumps(argv)
        return Library.toolkit.MaaToolkitRegisterCustomActionExecutor(
            inst.c_handle(),
            name.encode("utf-8"),
            str(exec_path).encode("utf-8"),
            json_argv.encode("utf-8"),
        )

    _api_properties_initialized: bool = False

    @classmethod
    def find_window(cls, class_name: str, window_name: str) -> List[MaaWin32Hwnd]:
        cls._set_api_properties()

        count = Library.toolkit.MaaToolkitFindWindow(
            class_name.encode("utf-8"), window_name.encode("utf-8")
        )

        windows = []
        for i in range(count):
            windows.append(Library.toolkit.MaaToolkitGetWindow(i))

        return windows

    @classmethod
    def search_window(cls, class_name: str, window_name: str) -> List[MaaWin32Hwnd]:
        cls._set_api_properties()

        count = Library.toolkit.MaaToolkitSearchWindow(
            class_name.encode("utf-8"), window_name.encode("utf-8")
        )

        windows = []
        for i in range(count):
            windows.append(Library.toolkit.MaaToolkitGetWindow(i))

        return windows

    @classmethod
    def get_cursor_window(cls) -> MaaWin32Hwnd:
        cls._set_api_properties()

        return Library.toolkit.MaaToolkitGetCursorWindow()

    @classmethod
    def get_desktop_window(cls) -> MaaWin32Hwnd:
        cls._set_api_properties()

        return Library.toolkit.MaaToolkitGetDesktopWindow()

    @classmethod
    def get_foreground_window(cls) -> MaaWin32Hwnd:
        cls._set_api_properties()

        return Library.toolkit.MaaToolkitGetForegroundWindow()

    @staticmethod
    def _set_api_properties():
        """
        Set the API properties.
        """

        if Toolkit._api_properties_initialized:
            return
        Toolkit._api_properties_initialized = True

        if not Library.initialized:
            raise RuntimeError(
                "Library not initialized, please call `library.open()` first."
            )
        if not Library.toolkit:
            raise RuntimeError(
                "Toolkit not initialized, please call `library.open()` with `toolkit=True`."
            )

        Library.toolkit.MaaToolkitInitOptionConfig.restype = MaaBool
        Library.toolkit.MaaToolkitInitOptionConfig.argtypes = [
            MaaStringView,
            MaaStringView,
        ]

        Library.toolkit.MaaToolkitPostFindDevice.restype = MaaBool
        Library.toolkit.MaaToolkitPostFindDevice.argtypes = None

        Library.toolkit.MaaToolkitIsFindDeviceCompleted.restype = MaaBool
        Library.toolkit.MaaToolkitIsFindDeviceCompleted.argtypes = None

        Library.toolkit.MaaToolkitGetDeviceCount.restype = MaaSize
        Library.toolkit.MaaToolkitGetDeviceCount.argtypes = None

        Library.toolkit.MaaToolkitGetDeviceName.restype = MaaStringView
        Library.toolkit.MaaToolkitGetDeviceName.argtypes = [MaaSize]

        Library.toolkit.MaaToolkitGetDeviceAdbPath.restype = MaaStringView
        Library.toolkit.MaaToolkitGetDeviceAdbPath.argtypes = [MaaSize]

        Library.toolkit.MaaToolkitGetDeviceAdbSerial.restype = MaaStringView
        Library.toolkit.MaaToolkitGetDeviceAdbSerial.argtypes = [MaaSize]

        Library.toolkit.MaaToolkitGetDeviceAdbControllerType.restype = (
            MaaAdbControllerType
        )

        Library.toolkit.MaaToolkitGetDeviceAdbControllerType.argtypes = [MaaSize]

        Library.toolkit.MaaToolkitGetDeviceAdbConfig.restype = MaaStringView
        Library.toolkit.MaaToolkitGetDeviceAdbConfig.argtypes = [MaaSize]

        Library.toolkit.MaaToolkitRegisterCustomRecognizerExecutor.restype = MaaBool
        Library.toolkit.MaaToolkitRegisterCustomRecognizerExecutor.argtypes = [
            MaaInstanceHandle,
            MaaStringView,
            MaaStringView,
            MaaStringView,
        ]

        Library.toolkit.MaaToolkitRegisterCustomActionExecutor.restype = MaaBool
        Library.toolkit.MaaToolkitRegisterCustomActionExecutor.argtypes = [
            MaaInstanceHandle,
            MaaStringView,
            MaaStringView,
            MaaStringView,
        ]

        Library.toolkit.MaaToolkitFindWindow.restype = MaaSize
        Library.toolkit.MaaToolkitFindWindow.argtypes = [
            MaaStringView,
            MaaStringView,
        ]

        Library.toolkit.MaaToolkitSearchWindow.restype = MaaSize
        Library.toolkit.MaaToolkitSearchWindow.argtypes = [
            MaaStringView,
            MaaStringView,
        ]

        Library.toolkit.MaaToolkitGetWindow.restype = MaaWin32Hwnd
        Library.toolkit.MaaToolkitGetWindow.argtypes = [MaaSize]

        Library.toolkit.MaaToolkitGetCursorWindow.restype = MaaWin32Hwnd
        Library.toolkit.MaaToolkitGetCursorWindow.argtypes = []

        Library.toolkit.MaaToolkitGetDesktopWindow.restype = MaaWin32Hwnd
        Library.toolkit.MaaToolkitGetDesktopWindow.argtypes = []

        Library.toolkit.MaaToolkitGetForegroundWindow.restype = MaaWin32Hwnd
        Library.toolkit.MaaToolkitGetForegroundWindow.argtypes = []
