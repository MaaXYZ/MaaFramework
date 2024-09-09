import ctypes
import json
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, List, Union

from .define import *
from .library import Library


@dataclass
class AdbDevice:
    name: str
    adb_path: Path
    address: str
    screencap_methods: MaaAdbScreencapMethod
    input_methods: MaaAdbInputMethod
    config: str


@dataclass
class DesktopWindow:
    hwnd: ctypes.c_void_p
    class_name: str
    window_name: str


class Toolkit:
    @classmethod
    def init_option(
        cls, user_path: Union[str, Path], default_config: Dict = {}
    ) -> bool:
        cls._set_api_properties()

        return bool(
            Library.toolkit.MaaToolkitConfigInitOption(
                str(user_path).encode("utf-8"),
                json.dumps(default_config, ensure_ascii=False).encode("utf-8"),
            )
        )

    @classmethod
    def find_adb_devices(
        cls, specified_adb: Union[str, Path] = None
    ) -> List[AdbDevice]:
        cls._set_api_properties()

        list_handle = Library.toolkit.MaaToolkitAdbDeviceListCreate()
        Library.toolkit.MaaToolkitAdbDeviceListDestroy(list_handle)

        if specified_adb:
            Library.toolkit.MaaToolkitAdbDeviceFindSpecified(
                str(specified_adb).encode("utf-8"), list_handle
            )
        else:
            Library.toolkit.MaaToolkitAdbDeviceFind(list_handle)

        count = Library.toolkit.MaaToolkitAdbDeviceListSize(list_handle)

        devices = []
        for i in range(count):
            device_handle = Library.toolkit.MaaToolkitAdbDeviceListAt(list_handle, i)

            name = Library.toolkit.MaaToolkitAdbDeviceGetName(device_handle).decode(
                "utf-8"
            )
            adb_path = Path(
                Library.toolkit.MaaToolkitAdbDeviceGetAdbPath(device_handle).decode(
                    "utf-8"
                )
            )
            address = Library.toolkit.MaaToolkitAdbDeviceGetAddress(
                device_handle
            ).decode("utf-8")
            screencap_methods = Library.toolkit.MaaToolkitAdbDeviceGetScreencapMethods(
                device_handle
            )
            input_methods = Library.toolkit.MaaToolkitAdbDeviceGetInputMethods(
                device_handle
            )
            config = Library.toolkit.MaaToolkitAdbDeviceGetConfig(device_handle).decode(
                "utf-8"
            )

            devices.append(
                AdbDevice(
                    name, adb_path, address, screencap_methods, input_methods, config
                )
            )

        return devices

    @classmethod
    def find_desktop_windows(cls) -> List[DesktopWindow]:
        cls._set_api_properties()

        list_handle = Library.toolkit.MaaToolkitDesktopWindowListCreate()
        Library.toolkit.MaaToolkitDesktopWindowListDestroy(list_handle)

        Library.toolkit.MaaToolkitDesktopWindowFindAll(list_handle)

        count = Library.toolkit.MaaToolkitDesktopWindowListSize(list_handle)

        windows = []
        for i in range(count):
            window_handle = Library.toolkit.MaaToolkitDesktopWindowListAt(
                list_handle, i
            )
            hwnd = Library.toolkit.MaaToolkitDesktopWindowGetHandle(window_handle)
            class_name = Library.toolkit.MaaToolkitDesktopWindowGetClassName(
                window_handle
            ).decode("utf-8")
            window_name = Library.toolkit.MaaToolkitDesktopWindowGetWindowName(
                window_handle
            ).decode("utf-8")

            windows.append(DesktopWindow(hwnd, class_name, window_name))

        return windows

    _api_properties_initialized: bool = False

    @staticmethod
    def _set_api_properties():
        if Toolkit._api_properties_initialized:
            return
        Toolkit._api_properties_initialized = True

        if not Library.initialized:
            raise RuntimeError(
                "Library not initialized, please call `library.open()` first."
            )

        Library.toolkit.MaaToolkitConfigInitOption.restype = MaaBool
        Library.toolkit.MaaToolkitConfigInitOption.argtypes = [
            ctypes.c_char_p,
            ctypes.c_char_p,
        ]

        Library.toolkit.MaaToolkitAdbDeviceListCreate.restype = (
            MaaToolkitAdbDeviceListHandle
        )
        Library.toolkit.MaaToolkitAdbDeviceListCreate.argtypes = []

        Library.toolkit.MaaToolkitAdbDeviceListDestroy.restype = None
        Library.toolkit.MaaToolkitAdbDeviceListDestroy.argtypes = [
            MaaToolkitAdbDeviceListHandle
        ]

        Library.toolkit.MaaToolkitAdbDeviceFind.restype = MaaBool
        Library.toolkit.MaaToolkitAdbDeviceFind.argtypes = [
            MaaToolkitAdbDeviceListHandle
        ]

        Library.toolkit.MaaToolkitAdbDeviceFindSpecified.restype = MaaBool
        Library.toolkit.MaaToolkitAdbDeviceFindSpecified.argtypes = [
            ctypes.c_char_p,
            MaaToolkitAdbDeviceListHandle,
        ]

        Library.toolkit.MaaToolkitAdbDeviceListSize.restype = MaaSize
        Library.toolkit.MaaToolkitAdbDeviceListSize.argtypes = [
            MaaToolkitAdbDeviceListHandle
        ]

        Library.toolkit.MaaToolkitAdbDeviceListAt.restype = MaaToolkitAdbDeviceHandle
        Library.toolkit.MaaToolkitAdbDeviceListAt.argtypes = [
            MaaToolkitAdbDeviceListHandle,
            MaaSize,
        ]

        Library.toolkit.MaaToolkitAdbDeviceGetName.restype = ctypes.c_char_p
        Library.toolkit.MaaToolkitAdbDeviceGetName.argtypes = [
            MaaToolkitAdbDeviceHandle
        ]

        Library.toolkit.MaaToolkitAdbDeviceGetAdbPath.restype = ctypes.c_char_p
        Library.toolkit.MaaToolkitAdbDeviceGetAdbPath.argtypes = [
            MaaToolkitAdbDeviceHandle
        ]

        Library.toolkit.MaaToolkitAdbDeviceGetAddress.restype = ctypes.c_char_p
        Library.toolkit.MaaToolkitAdbDeviceGetAddress.argtypes = [
            MaaToolkitAdbDeviceHandle
        ]

        Library.toolkit.MaaToolkitAdbDeviceGetScreencapMethods.restype = (
            MaaAdbScreencapMethod
        )
        Library.toolkit.MaaToolkitAdbDeviceGetScreencapMethods.argtypes = [
            MaaToolkitAdbDeviceHandle
        ]

        Library.toolkit.MaaToolkitAdbDeviceGetInputMethods.restype = MaaAdbInputMethod
        Library.toolkit.MaaToolkitAdbDeviceGetInputMethods.argtypes = [
            MaaToolkitAdbDeviceHandle
        ]

        Library.toolkit.MaaToolkitAdbDeviceGetConfig.restype = ctypes.c_char_p
        Library.toolkit.MaaToolkitAdbDeviceGetConfig.argtypes = [
            MaaToolkitAdbDeviceHandle
        ]

        Library.toolkit.MaaToolkitDesktopWindowListCreate.restype = (
            MaaToolkitDesktopWindowListHandle
        )
        Library.toolkit.MaaToolkitDesktopWindowListCreate.argtypes = []

        Library.toolkit.MaaToolkitDesktopWindowListDestroy.restype = None
        Library.toolkit.MaaToolkitDesktopWindowListDestroy.argtypes = [
            MaaToolkitDesktopWindowListHandle
        ]

        Library.toolkit.MaaToolkitDesktopWindowFindAll.restype = MaaBool
        Library.toolkit.MaaToolkitDesktopWindowFindAll.argtypes = [
            MaaToolkitDesktopWindowListHandle
        ]

        Library.toolkit.MaaToolkitDesktopWindowListSize.restype = MaaSize
        Library.toolkit.MaaToolkitDesktopWindowListSize.argtypes = [
            MaaToolkitDesktopWindowListHandle
        ]

        Library.toolkit.MaaToolkitDesktopWindowListAt.restype = (
            MaaToolkitDesktopWindowHandle
        )
        Library.toolkit.MaaToolkitDesktopWindowListAt.argtypes = [
            MaaToolkitDesktopWindowListHandle,
            MaaSize,
        ]

        Library.toolkit.MaaToolkitDesktopWindowGetHandle.restype = ctypes.c_void_p
        Library.toolkit.MaaToolkitDesktopWindowGetHandle.argtypes = [
            MaaToolkitDesktopWindowHandle
        ]

        Library.toolkit.MaaToolkitDesktopWindowGetClassName.restype = ctypes.c_char_p
        Library.toolkit.MaaToolkitDesktopWindowGetClassName.argtypes = [
            MaaToolkitDesktopWindowHandle
        ]

        Library.toolkit.MaaToolkitDesktopWindowGetWindowName.restype = ctypes.c_char_p
        Library.toolkit.MaaToolkitDesktopWindowGetWindowName.argtypes = [
            MaaToolkitDesktopWindowHandle
        ]
