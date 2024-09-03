import asyncio
import ctypes
import json
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, List, Union

from .define import *
from .tasker import Tasker
from .library import Library
from .buffer import StringBuffer


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

        return bool(
            Library.toolkit.MaaToolkitInitOptionConfig(
                str(user_path).encode("utf-8"),
                json.dumps(default_config, ensure_ascii=False).encode("utf-8"),
            )
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
        cls, inst: Tasker, name: str, exec_path: Union[str, Path], argv: List[str]
    ) -> bool:
        """
        Register a recognizer exec agent.
        """
        cls._set_api_properties()

        argv_list = [str(arg).encode("utf-8") for arg in argv]

        return bool(
            Library.toolkit.MaaToolkitRegisterCustomRecognizerExecutor(
                inst._handle,
                name.encode("utf-8"),
                str(exec_path).encode("utf-8"),
                (ctypes.c_char_p * len(argv_list))(*argv_list),
                len(argv_list),
            )
        )

    @classmethod
    def register_action_exec_agent(
        cls, inst: Tasker, name: str, exec_path: Union[str, Path], argv: List[str]
    ) -> bool:
        """
        Register a action exec agent.
        """
        cls._set_api_properties()

        argv_list = [str(arg).encode("utf-8") for arg in argv]

        return bool(
            Library.toolkit.MaaToolkitRegisterCustomActionExecutor(
                inst._handle(),
                name.encode("utf-8"),
                str(exec_path).encode("utf-8"),
                (ctypes.c_char_p * len(argv_list))(*argv_list),
            )
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
    def search_window(cls, class_regex: str, window_regex: str) -> List[MaaWin32Hwnd]:
        cls._set_api_properties()

        count = Library.toolkit.MaaToolkitSearchWindow(
            class_regex.encode("utf-8"), window_regex.encode("utf-8")
        )

        windows = []
        for i in range(count):
            windows.append(Library.toolkit.MaaToolkitGetWindow(i))

        return windows

    @classmethod
    def list_windows(cls) -> List[MaaWin32Hwnd]:
        cls._set_api_properties()

        count = Library.toolkit.MaaToolkitListWindows()

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

    @classmethod
    def get_class_name(cls, hwnd: MaaWin32Hwnd) -> str:
        cls._set_api_properties()

        buffer = StringBuffer()
        Library.toolkit.MaaToolkitGetWindowClassName(hwnd, buffer._handle)
        return buffer.get()

    @classmethod
    def get_window_name(cls, hwnd: MaaWin32Hwnd) -> str:
        cls._set_api_properties()

        buffer = StringBuffer()
        Library.toolkit.MaaToolkitGetWindowWindowName(hwnd, buffer._handle)
        return buffer.get()

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
            ctypes.c_char_p,
            ctypes.c_char_p,
        ]

        Library.toolkit.MaaToolkitPostFindDevice.restype = MaaBool
        Library.toolkit.MaaToolkitPostFindDevice.argtypes = None

        Library.toolkit.MaaToolkitIsFindDeviceCompleted.restype = MaaBool
        Library.toolkit.MaaToolkitIsFindDeviceCompleted.argtypes = None

        Library.toolkit.MaaToolkitGetDeviceCount.restype = MaaSize
        Library.toolkit.MaaToolkitGetDeviceCount.argtypes = None

        Library.toolkit.MaaToolkitGetDeviceName.restype = ctypes.c_char_p
        Library.toolkit.MaaToolkitGetDeviceName.argtypes = [MaaSize]

        Library.toolkit.MaaToolkitGetDeviceAdbPath.restype = ctypes.c_char_p
        Library.toolkit.MaaToolkitGetDeviceAdbPath.argtypes = [MaaSize]

        Library.toolkit.MaaToolkitGetDeviceAdbSerial.restype = ctypes.c_char_p
        Library.toolkit.MaaToolkitGetDeviceAdbSerial.argtypes = [MaaSize]

        Library.toolkit.MaaToolkitGetDeviceAdbControllerType.restype = (
            MaaAdbControllerType
        )

        Library.toolkit.MaaToolkitGetDeviceAdbControllerType.argtypes = [MaaSize]

        Library.toolkit.MaaToolkitGetDeviceAdbConfig.restype = ctypes.c_char_p
        Library.toolkit.MaaToolkitGetDeviceAdbConfig.argtypes = [MaaSize]

        Library.toolkit.MaaToolkitRegisterCustomRecognizerExecutor.restype = MaaBool
        Library.toolkit.MaaToolkitRegisterCustomRecognizerExecutor.argtypes = [
            MaaTaskerHandle,
            ctypes.c_char_p,
            ctypes.c_char_p,
            ctypes.POINTER(ctypes.c_char_p),
            MaaSize,
        ]

        Library.toolkit.MaaToolkitRegisterCustomActionExecutor.restype = MaaBool
        Library.toolkit.MaaToolkitRegisterCustomActionExecutor.argtypes = [
            MaaTaskerHandle,
            ctypes.c_char_p,
            ctypes.c_char_p,
            ctypes.POINTER(ctypes.c_char_p),
            MaaSize,
        ]

        Library.toolkit.MaaToolkitFindWindow.restype = MaaSize
        Library.toolkit.MaaToolkitFindWindow.argtypes = [
            ctypes.c_char_p,
            ctypes.c_char_p,
        ]

        Library.toolkit.MaaToolkitSearchWindow.restype = MaaSize
        Library.toolkit.MaaToolkitSearchWindow.argtypes = [
            ctypes.c_char_p,
            ctypes.c_char_p,
        ]

        Library.toolkit.MaaToolkitListWindows.restype = MaaSize
        Library.toolkit.MaaToolkitListWindows.argtypes = None

        Library.toolkit.MaaToolkitGetWindow.restype = MaaWin32Hwnd
        Library.toolkit.MaaToolkitGetWindow.argtypes = [MaaSize]

        Library.toolkit.MaaToolkitGetCursorWindow.restype = MaaWin32Hwnd
        Library.toolkit.MaaToolkitGetCursorWindow.argtypes = []

        Library.toolkit.MaaToolkitGetDesktopWindow.restype = MaaWin32Hwnd
        Library.toolkit.MaaToolkitGetDesktopWindow.argtypes = []

        Library.toolkit.MaaToolkitGetForegroundWindow.restype = MaaWin32Hwnd
        Library.toolkit.MaaToolkitGetForegroundWindow.argtypes = []

        Library.toolkit.MaaToolkitGetWindowClassName.restype = MaaBool
        Library.toolkit.MaaToolkitGetWindowClassName.argtypes = [
            MaaWin32Hwnd,
            MaaStringBufferHandle,
        ]

        Library.toolkit.MaaToolkitGetWindowWindowName.restype = MaaBool
        Library.toolkit.MaaToolkitGetWindowWindowName.argtypes = [
            MaaWin32Hwnd,
            MaaStringBufferHandle,
        ]
