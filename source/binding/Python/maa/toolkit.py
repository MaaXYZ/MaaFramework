import ctypes
import json
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, List, Union, Optional, Any

from .define import *
from .library import Library


@dataclass
class AdbDevice:
    name: str
    adb_path: Path
    address: str
    screencap_methods: int
    input_methods: int
    config: Dict[str, Any]


@dataclass
class DesktopWindow:
    hwnd: ctypes.c_void_p
    class_name: str
    window_name: str


class Toolkit:

    ### public ###

    @staticmethod
    def init_option(user_path: Union[str, Path], default_config: Dict = {}) -> bool:
        """从 user_path 中加载全局配置 / Load global config from user_path

        Args:
            user_path: 配置存储路径 / Config storage path
            default_config: 默认配置 / Default config

        Returns:
            bool: 是否成功 / Whether successful
        """
        Toolkit._set_api_properties()

        return bool(
            Library.toolkit().MaaToolkitConfigInitOption(
                str(user_path).encode(),
                json.dumps(default_config, ensure_ascii=False).encode(),
            )
        )

    @staticmethod
    def find_adb_devices(
        specified_adb: Optional[Union[str, Path]] = None
    ) -> List[AdbDevice]:
        """搜索所有已知安卓模拟器 / Search all known Android emulators

        Args:
            specified_adb: 可选，指定 adb 路径进行搜索 / Optional, search using specified adb path

        Returns:
            List[AdbDevice]: 设备列表 / Device list
        """
        Toolkit._set_api_properties()

        list_handle = Library.toolkit().MaaToolkitAdbDeviceListCreate()

        if specified_adb:
            Library.toolkit().MaaToolkitAdbDeviceFindSpecified(
                str(specified_adb).encode(), list_handle
            )
        else:
            Library.toolkit().MaaToolkitAdbDeviceFind(list_handle)

        count = Library.toolkit().MaaToolkitAdbDeviceListSize(list_handle)

        devices = []
        for i in range(count):
            device_handle = Library.toolkit().MaaToolkitAdbDeviceListAt(list_handle, i)

            name = Library.toolkit().MaaToolkitAdbDeviceGetName(device_handle).decode()
            adb_path = Path(
                Library.toolkit().MaaToolkitAdbDeviceGetAdbPath(device_handle).decode()
            )
            address = (
                Library.toolkit().MaaToolkitAdbDeviceGetAddress(device_handle).decode()
            )
            screencap_methods = int(
                Library.toolkit().MaaToolkitAdbDeviceGetScreencapMethods(device_handle)
            )
            input_methods = int(
                Library.toolkit().MaaToolkitAdbDeviceGetInputMethods(device_handle)
            )
            config = json.loads(
                Library.toolkit().MaaToolkitAdbDeviceGetConfig(device_handle).decode()
            )

            devices.append(
                AdbDevice(
                    name, adb_path, address, screencap_methods, input_methods, config
                )
            )

        Library.toolkit().MaaToolkitAdbDeviceListDestroy(list_handle)

        return devices

    @staticmethod
    def find_desktop_windows() -> List[DesktopWindow]:
        """查询所有窗口信息 / Query all window info

        Returns:
            List[DesktopWindow]: 窗口列表 / Window list
        """
        Toolkit._set_api_properties()

        list_handle = Library.toolkit().MaaToolkitDesktopWindowListCreate()

        Library.toolkit().MaaToolkitDesktopWindowFindAll(list_handle)

        count = Library.toolkit().MaaToolkitDesktopWindowListSize(list_handle)

        windows = []
        for i in range(count):
            window_handle = Library.toolkit().MaaToolkitDesktopWindowListAt(
                list_handle, i
            )
            hwnd = Library.toolkit().MaaToolkitDesktopWindowGetHandle(window_handle)
            class_name = (
                Library.toolkit()
                .MaaToolkitDesktopWindowGetClassName(window_handle)
                .decode()
            )
            window_name = (
                Library.toolkit()
                .MaaToolkitDesktopWindowGetWindowName(window_handle)
                .decode()
            )

            windows.append(DesktopWindow(hwnd, class_name, window_name))

        Library.toolkit().MaaToolkitDesktopWindowListDestroy(list_handle)
        return windows

    ### private ###

    _api_properties_initialized: bool = False

    @staticmethod
    def _set_api_properties():
        if Toolkit._api_properties_initialized:
            return
        Toolkit._api_properties_initialized = True

        Library.toolkit().MaaToolkitConfigInitOption.restype = MaaBool
        Library.toolkit().MaaToolkitConfigInitOption.argtypes = [
            ctypes.c_char_p,
            ctypes.c_char_p,
        ]

        Library.toolkit().MaaToolkitAdbDeviceListCreate.restype = (
            MaaToolkitAdbDeviceListHandle
        )
        Library.toolkit().MaaToolkitAdbDeviceListCreate.argtypes = []

        Library.toolkit().MaaToolkitAdbDeviceListDestroy.restype = None
        Library.toolkit().MaaToolkitAdbDeviceListDestroy.argtypes = [
            MaaToolkitAdbDeviceListHandle
        ]

        Library.toolkit().MaaToolkitAdbDeviceFind.restype = MaaBool
        Library.toolkit().MaaToolkitAdbDeviceFind.argtypes = [
            MaaToolkitAdbDeviceListHandle
        ]

        Library.toolkit().MaaToolkitAdbDeviceFindSpecified.restype = MaaBool
        Library.toolkit().MaaToolkitAdbDeviceFindSpecified.argtypes = [
            ctypes.c_char_p,
            MaaToolkitAdbDeviceListHandle,
        ]

        Library.toolkit().MaaToolkitAdbDeviceListSize.restype = MaaSize
        Library.toolkit().MaaToolkitAdbDeviceListSize.argtypes = [
            MaaToolkitAdbDeviceListHandle
        ]

        Library.toolkit().MaaToolkitAdbDeviceListAt.restype = MaaToolkitAdbDeviceHandle
        Library.toolkit().MaaToolkitAdbDeviceListAt.argtypes = [
            MaaToolkitAdbDeviceListHandle,
            MaaSize,
        ]

        Library.toolkit().MaaToolkitAdbDeviceGetName.restype = ctypes.c_char_p
        Library.toolkit().MaaToolkitAdbDeviceGetName.argtypes = [
            MaaToolkitAdbDeviceHandle
        ]

        Library.toolkit().MaaToolkitAdbDeviceGetAdbPath.restype = ctypes.c_char_p
        Library.toolkit().MaaToolkitAdbDeviceGetAdbPath.argtypes = [
            MaaToolkitAdbDeviceHandle
        ]

        Library.toolkit().MaaToolkitAdbDeviceGetAddress.restype = ctypes.c_char_p
        Library.toolkit().MaaToolkitAdbDeviceGetAddress.argtypes = [
            MaaToolkitAdbDeviceHandle
        ]

        Library.toolkit().MaaToolkitAdbDeviceGetScreencapMethods.restype = (
            MaaAdbScreencapMethod
        )
        Library.toolkit().MaaToolkitAdbDeviceGetScreencapMethods.argtypes = [
            MaaToolkitAdbDeviceHandle
        ]

        Library.toolkit().MaaToolkitAdbDeviceGetInputMethods.restype = MaaAdbInputMethod
        Library.toolkit().MaaToolkitAdbDeviceGetInputMethods.argtypes = [
            MaaToolkitAdbDeviceHandle
        ]

        Library.toolkit().MaaToolkitAdbDeviceGetConfig.restype = ctypes.c_char_p
        Library.toolkit().MaaToolkitAdbDeviceGetConfig.argtypes = [
            MaaToolkitAdbDeviceHandle
        ]

        Library.toolkit().MaaToolkitDesktopWindowListCreate.restype = (
            MaaToolkitDesktopWindowListHandle
        )
        Library.toolkit().MaaToolkitDesktopWindowListCreate.argtypes = []

        Library.toolkit().MaaToolkitDesktopWindowListDestroy.restype = None
        Library.toolkit().MaaToolkitDesktopWindowListDestroy.argtypes = [
            MaaToolkitDesktopWindowListHandle
        ]

        Library.toolkit().MaaToolkitDesktopWindowFindAll.restype = MaaBool
        Library.toolkit().MaaToolkitDesktopWindowFindAll.argtypes = [
            MaaToolkitDesktopWindowListHandle
        ]

        Library.toolkit().MaaToolkitDesktopWindowListSize.restype = MaaSize
        Library.toolkit().MaaToolkitDesktopWindowListSize.argtypes = [
            MaaToolkitDesktopWindowListHandle
        ]

        Library.toolkit().MaaToolkitDesktopWindowListAt.restype = (
            MaaToolkitDesktopWindowHandle
        )
        Library.toolkit().MaaToolkitDesktopWindowListAt.argtypes = [
            MaaToolkitDesktopWindowListHandle,
            MaaSize,
        ]

        Library.toolkit().MaaToolkitDesktopWindowGetHandle.restype = ctypes.c_void_p
        Library.toolkit().MaaToolkitDesktopWindowGetHandle.argtypes = [
            MaaToolkitDesktopWindowHandle
        ]

        Library.toolkit().MaaToolkitDesktopWindowGetClassName.restype = ctypes.c_char_p
        Library.toolkit().MaaToolkitDesktopWindowGetClassName.argtypes = [
            MaaToolkitDesktopWindowHandle
        ]

        Library.toolkit().MaaToolkitDesktopWindowGetWindowName.restype = ctypes.c_char_p
        Library.toolkit().MaaToolkitDesktopWindowGetWindowName.argtypes = [
            MaaToolkitDesktopWindowHandle
        ]
