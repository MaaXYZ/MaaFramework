import ctypes
import json
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Callable, Optional, Union

from .define import *
from .library import Library


_IOSCancelCallback = FUNCTYPE(MaaBool, ctypes.c_void_p)


@dataclass
class AdbDevice:
    """ADB 设备信息 / ADB device information

    通过 Toolkit.find_adb_devices 获取。
    Obtained via Toolkit.find_adb_devices.

    Attributes:
        name: 设备名称 / Device name
        adb_path: adb 可执行文件路径 / Path to adb executable
        address: 设备地址 (如 127.0.0.1:5555) / Device address (e.g., 127.0.0.1:5555)
        screencap_methods: 可用的截图方式位掩码 / Available screenshot methods bitmask
        input_methods: 可用的输入方式位掩码 / Available input methods bitmask
        config: 额外配置信息 / Extra configuration
    """

    name: str
    adb_path: Path
    address: str
    screencap_methods: int
    input_methods: int
    config: dict[str, Any]


@dataclass
class IOSDevice:
    """iOS discovery result; connections retain every USB/Wi-Fi candidate."""

    name: str
    udid: str
    screencap_methods: int
    connections: list[dict[str, Any]]
    state: str


@dataclass
class DesktopWindow:
    """桌面窗口信息 / Desktop window information

    通过 Toolkit.find_desktop_windows 获取。
    Obtained via Toolkit.find_desktop_windows.

    Attributes:
        hwnd: 窗口句柄 / Window handle
        class_name: 窗口类名 / Window class name
        window_name: 窗口标题 / Window title
    """

    hwnd: ctypes.c_void_p
    class_name: str
    window_name: str


@dataclass
class GamescopeInstance:
    """gamescope 实例信息 / gamescope instance info

    通过 Toolkit.find_gamescope_instances 获取。
    Obtained via Toolkit.find_gamescope_instances.

    Attributes:
        display_no: display 编号 (gamescope-<n> 的 n) / display number (n in gamescope-<n>)
        pipewire_node_id: PipeWire 节点 ID (0 表示无可用截图节点) / PipeWire node ID (0 means no capture node)
        eis_socket_path: EIS socket 路径 (空串表示无 EIS socket) / EIS socket path (empty means no EIS socket)
    """

    display_no: int
    pipewire_node_id: int
    eis_socket_path: str


class PortalHelper:
    _handle: MaaToolkitPortalHelperHandle

    def __init__(self, handle: MaaToolkitPortalHelperHandle):
        self._handle = handle

    def __del__(self):
        if self._handle:
            Library.toolkit().MaaToolkitPortalHelperDestroy(self._handle)

    def open_stream(self) -> bool:
        return Library.toolkit().MaaToolkitPortalHelperOpenStream(self._handle)

    def get_persist(self) -> bool:
        return bool(Library.toolkit().MaaToolkitPortalHelperGetPersist(self._handle))

    def set_persist(self, enable: bool):
        Library.toolkit().MaaToolkitPortalHelperSetPersist(self._handle, enable)

    def get_pipewire_fd(self) -> int:
        return Library.toolkit().MaaToolkitPortalHelperGetPipeWireFD(self._handle)

    def get_pipewire_node_id(self) -> int:
        return Library.toolkit().MaaToolkitPortalHelperGetPipeWireNodeID(self._handle)

    def get_restore_token(self) -> Optional[str]:
        token = Library.toolkit().MaaToolkitPortalHelperGetRestoreToken(self._handle)
        return token.decode() if token else None

    def set_restore_token(self, token: str):
        Library.toolkit().MaaToolkitPortalHelperSetRestoreToken(self._handle, token.encode("utf-8"))


class Toolkit:
    """工具包 / Toolkit

    提供设备发现、配置初始化等辅助功能。
    Provides auxiliary functions such as device discovery and configuration initialization.
    """

    ### public ###

    @staticmethod
    def init_option(user_path: Union[str, Path], default_config: Optional[dict[str, Any]] = None) -> bool:
        """从 user_path 中加载全局配置 / Load global config from user_path

        Args:
            user_path: 配置存储路径 / Config storage path
            default_config: 默认配置 / Default config

        Returns:
            bool: 是否成功 / Whether successful
        """
        if default_config is None:
            default_config = {}
        if Library.is_agent_server():
            from .tasker import Tasker

            print("Warning: Toolkit.init_option is deprecated in AgentServer; only set_log_dir is applied.")
            config = default_config
            log_dir = Path(user_path) / "debug" if config.get("logging", True) else ""
            return Tasker.set_log_dir(log_dir)

        Toolkit._set_api_properties()

        return bool(
            Library.toolkit().MaaToolkitConfigInitOption(
                str(user_path).encode(),
                json.dumps(default_config, ensure_ascii=False).encode(),
            )
        )

    @staticmethod
    def find_adb_devices(specified_adb: Optional[Union[str, Path]] = None) -> list[AdbDevice]:
        """搜索所有已知安卓模拟器 / Search all known Android emulators

        Args:
            specified_adb: 可选，指定 adb 路径进行搜索 / Optional, search using specified adb path

        Returns:
            List[AdbDevice]: 设备列表 / Device list
        """
        Toolkit._set_api_properties()

        list_handle = Library.toolkit().MaaToolkitAdbDeviceListCreate()

        if specified_adb:
            Library.toolkit().MaaToolkitAdbDeviceFindSpecified(str(specified_adb).encode(), list_handle)
        else:
            Library.toolkit().MaaToolkitAdbDeviceFind(list_handle)

        count = Library.toolkit().MaaToolkitAdbDeviceListSize(list_handle)

        devices: list[AdbDevice] = []
        for i in range(count):
            device_handle = Library.toolkit().MaaToolkitAdbDeviceListAt(list_handle, i)

            name = Library.toolkit().MaaToolkitAdbDeviceGetName(device_handle).decode()
            adb_path = Path(Library.toolkit().MaaToolkitAdbDeviceGetAdbPath(device_handle).decode())
            address = Library.toolkit().MaaToolkitAdbDeviceGetAddress(device_handle).decode()
            screencap_methods = int(Library.toolkit().MaaToolkitAdbDeviceGetScreencapMethods(device_handle))
            input_methods = int(Library.toolkit().MaaToolkitAdbDeviceGetInputMethods(device_handle))
            config = json.loads(Library.toolkit().MaaToolkitAdbDeviceGetConfig(device_handle).decode())

            devices.append(AdbDevice(name, adb_path, address, screencap_methods, input_methods, config))

        Library.toolkit().MaaToolkitAdbDeviceListDestroy(list_handle)

        return devices

    @staticmethod
    def find_ios_devices(
        timeout_ms: int = 3000,
        config: Optional[dict[str, Any]] = None,
        connection_method: MaaIOSConnectionMethodEnum = MaaIOSConnectionMethodEnum.Auto,
        cancelled: Optional[Callable[[], bool]] = None,
    ) -> list[IOSDevice]:
        """Discover USB/Wi-Fi devices without pairing or starting media.

        cancelled may run on a native worker thread; it must be thread-safe.
        Callback exceptions are re-raised after native work has stopped.
        Device state is discovered/paired/connected, not a service health check.
        """
        if timeout_ms < 0 or timeout_ms > 60000:
            raise ValueError("timeout_ms must be between 0 and 60000")
        if config is not None and not isinstance(config, dict):
            raise TypeError("xdb server config must be a dict")
        if cancelled is not None and not callable(cancelled):
            raise TypeError("cancelled must be callable")
        errors: list[BaseException] = []

        def cancel(_userdata: Any) -> bool:
            if errors:
                return True
            try:
                return bool(cancelled())
            except BaseException as error:
                errors.append(error)
                return True

        # The native call joins its worker before these borrowed objects are released.
        cancel_callback = _IOSCancelCallback(cancel) if cancelled is not None else _IOSCancelCallback()
        Toolkit._set_ios_api_properties()
        api = Library.toolkit()
        handle = api.MaaToolkitIOSDeviceListCreate()
        if not handle:
            raise MemoryError("could not allocate iOS device list")
        try:
            options = json.dumps({**(config or {}), "timeout_ms": timeout_ms}).encode()
            success = api.MaaToolkitIOSDeviceFindWithOptions(connection_method, options, cancel_callback, None, handle)
            if errors:
                raise errors[0]
            if not success:
                warnings = api.MaaToolkitIOSDeviceListGetWarnings(handle).decode("utf-8")
                raise RuntimeError(f"iOS discovery failed: {warnings}")
            devices = []
            for index in range(api.MaaToolkitIOSDeviceListSize(handle)):
                device = api.MaaToolkitIOSDeviceListAt(handle, index)
                devices.append(IOSDevice(
                    api.MaaToolkitIOSDeviceGetName(device).decode("utf-8"),
                    api.MaaToolkitIOSDeviceGetUdid(device).decode("utf-8"),
                    int(api.MaaToolkitIOSDeviceGetScreencapMethods(device)),
                    json.loads(api.MaaToolkitIOSDeviceGetConnections(device).decode("utf-8")),
                    api.MaaToolkitIOSDeviceGetState(device).decode("utf-8"),
                ))
            return devices
        finally:
            api.MaaToolkitIOSDeviceListDestroy(handle)

    @staticmethod
    def find_desktop_windows() -> list[DesktopWindow]:
        """查询所有窗口信息 / Query all window info

        Returns:
            List[DesktopWindow]: 窗口列表 / Window list
        """
        Toolkit._set_api_properties()

        list_handle = Library.toolkit().MaaToolkitDesktopWindowListCreate()

        Library.toolkit().MaaToolkitDesktopWindowFindAll(list_handle)

        count = Library.toolkit().MaaToolkitDesktopWindowListSize(list_handle)

        windows: list[DesktopWindow] = []
        for i in range(count):
            window_handle = Library.toolkit().MaaToolkitDesktopWindowListAt(list_handle, i)
            hwnd = Library.toolkit().MaaToolkitDesktopWindowGetHandle(window_handle)
            class_name = Library.toolkit().MaaToolkitDesktopWindowGetClassName(window_handle).decode()
            window_name = Library.toolkit().MaaToolkitDesktopWindowGetWindowName(window_handle).decode()

            windows.append(DesktopWindow(hwnd, class_name, window_name))

        Library.toolkit().MaaToolkitDesktopWindowListDestroy(list_handle)
        return windows

    @staticmethod
    def find_gamescope_instances() -> list[GamescopeInstance]:
        """查找 gamescope 实例 / Find gamescope instances

        Returns:
            List[GamescopeInstance]: 实例列表 (gamescope 未运行时为空) / Instance list (empty when gamescope is not running)
        """
        Toolkit._set_api_properties()

        list_handle = Library.toolkit().MaaToolkitGamescopeInstanceListCreate()

        Library.toolkit().MaaToolkitGamescopeInstanceFindAll(list_handle)

        count = Library.toolkit().MaaToolkitGamescopeInstanceListSize(list_handle)

        instances: list[GamescopeInstance] = []
        for i in range(count):
            instance_handle = Library.toolkit().MaaToolkitGamescopeInstanceListAt(list_handle, i)
            display_no = Library.toolkit().MaaToolkitGamescopeInstanceGetDisplayNo(instance_handle)
            node_id = Library.toolkit().MaaToolkitGamescopeInstanceGetPipeWireNodeId(instance_handle)
            eis_socket_path = Library.toolkit().MaaToolkitGamescopeInstanceGetEisSocketPath(instance_handle).decode()

            instances.append(GamescopeInstance(display_no, node_id, eis_socket_path))

        Library.toolkit().MaaToolkitGamescopeInstanceListDestroy(list_handle)
        return instances

    @staticmethod
    def macos_check_permission(perm: MaaMacOSPermissionEnum) -> bool:
        """检查 macOS 权限 / Check macOS permission

        检查应用是否已获得指定的 macOS 系统权限。
        Check if the application has been granted the specified macOS system permission.

        Args:
            perm: 权限类型 / Permission type

        Returns:
            bool: 是否已授权 / Whether permission is granted
        """
        Toolkit._set_api_properties()

        return bool(Library.toolkit().MaaToolkitMacOSCheckPermission(perm))

    @staticmethod
    def macos_request_permission(perm: MaaMacOSPermissionEnum) -> bool:
        """请求 macOS 权限 / Request macOS permission

        向用户请求指定的 macOS 系统权限。系统可能会弹出授权对话框。
        Request the specified macOS system permission from user. System may show an authorization
        dialog.

        Args:
            perm: 权限类型 / Permission type

        Returns:
            bool: 是否成功请求（不代表已授权）/ Whether request succeeded (doesn't mean granted)
        """
        Toolkit._set_api_properties()

        return bool(Library.toolkit().MaaToolkitMacOSRequestPermission(perm))

    @staticmethod
    def macos_reveal_permission_settings(perm: MaaMacOSPermissionEnum) -> bool:
        """打开 macOS 权限设置 / Open macOS permission settings

        打开系统偏好设置中对应权限的设置页面。
        Open the corresponding permission settings page in System Preferences.

        Args:
            perm: 权限类型 / Permission type

        Returns:
            bool: 是否成功打开 / Whether successfully opened
        """
        Toolkit._set_api_properties()

        return bool(Library.toolkit().MaaToolkitMacOSRevealPermissionSettings(perm))

    @staticmethod
    def portal_helper_create() -> PortalHelper:
        Toolkit._set_api_properties()

        portal_helper_handle = Library.toolkit().MaaToolkitPortalHelperCreate()
        if not portal_helper_handle:
            raise RuntimeError("Failed to create PortalHelper.")
        return PortalHelper(portal_helper_handle)

    ### private ###

    _api_properties_initialized: bool = False

    @staticmethod
    def _set_api_properties():
        if Toolkit._api_properties_initialized:
            return

        with Library._api_lock:
            if Toolkit._api_properties_initialized:
                return

            Toolkit._assign_api_properties()
            Toolkit._api_properties_initialized = True

    @staticmethod
    def _assign_api_properties() -> None:
        Library.toolkit().MaaToolkitConfigInitOption.restype = MaaBool
        Library.toolkit().MaaToolkitConfigInitOption.argtypes = [
            ctypes.c_char_p,
            ctypes.c_char_p,
        ]

        Library.toolkit().MaaToolkitAdbDeviceListCreate.restype = MaaToolkitAdbDeviceListHandle
        Library.toolkit().MaaToolkitAdbDeviceListCreate.argtypes = []

        Library.toolkit().MaaToolkitAdbDeviceListDestroy.restype = None
        Library.toolkit().MaaToolkitAdbDeviceListDestroy.argtypes = [MaaToolkitAdbDeviceListHandle]

        Library.toolkit().MaaToolkitAdbDeviceFind.restype = MaaBool
        Library.toolkit().MaaToolkitAdbDeviceFind.argtypes = [MaaToolkitAdbDeviceListHandle]

        Library.toolkit().MaaToolkitAdbDeviceFindSpecified.restype = MaaBool
        Library.toolkit().MaaToolkitAdbDeviceFindSpecified.argtypes = [
            ctypes.c_char_p,
            MaaToolkitAdbDeviceListHandle,
        ]

        Library.toolkit().MaaToolkitAdbDeviceListSize.restype = MaaSize
        Library.toolkit().MaaToolkitAdbDeviceListSize.argtypes = [MaaToolkitAdbDeviceListHandle]

        Library.toolkit().MaaToolkitAdbDeviceListAt.restype = MaaToolkitAdbDeviceHandle
        Library.toolkit().MaaToolkitAdbDeviceListAt.argtypes = [
            MaaToolkitAdbDeviceListHandle,
            MaaSize,
        ]

        Library.toolkit().MaaToolkitAdbDeviceGetName.restype = ctypes.c_char_p
        Library.toolkit().MaaToolkitAdbDeviceGetName.argtypes = [MaaToolkitAdbDeviceHandle]

        Library.toolkit().MaaToolkitAdbDeviceGetAdbPath.restype = ctypes.c_char_p
        Library.toolkit().MaaToolkitAdbDeviceGetAdbPath.argtypes = [MaaToolkitAdbDeviceHandle]

        Library.toolkit().MaaToolkitAdbDeviceGetAddress.restype = ctypes.c_char_p
        Library.toolkit().MaaToolkitAdbDeviceGetAddress.argtypes = [MaaToolkitAdbDeviceHandle]

        Library.toolkit().MaaToolkitAdbDeviceGetScreencapMethods.restype = MaaAdbScreencapMethod
        Library.toolkit().MaaToolkitAdbDeviceGetScreencapMethods.argtypes = [MaaToolkitAdbDeviceHandle]

        Library.toolkit().MaaToolkitAdbDeviceGetInputMethods.restype = MaaAdbInputMethod
        Library.toolkit().MaaToolkitAdbDeviceGetInputMethods.argtypes = [MaaToolkitAdbDeviceHandle]

        Library.toolkit().MaaToolkitAdbDeviceGetConfig.restype = ctypes.c_char_p
        Library.toolkit().MaaToolkitAdbDeviceGetConfig.argtypes = [MaaToolkitAdbDeviceHandle]

        Library.toolkit().MaaToolkitGamescopeInstanceListCreate.restype = MaaToolkitGamescopeInstanceListHandle
        Library.toolkit().MaaToolkitGamescopeInstanceListCreate.argtypes = []

        Library.toolkit().MaaToolkitGamescopeInstanceListDestroy.restype = None
        Library.toolkit().MaaToolkitGamescopeInstanceListDestroy.argtypes = [MaaToolkitGamescopeInstanceListHandle]

        Library.toolkit().MaaToolkitGamescopeInstanceFindAll.restype = MaaBool
        Library.toolkit().MaaToolkitGamescopeInstanceFindAll.argtypes = [MaaToolkitGamescopeInstanceListHandle]

        Library.toolkit().MaaToolkitGamescopeInstanceListSize.restype = MaaSize
        Library.toolkit().MaaToolkitGamescopeInstanceListSize.argtypes = [MaaToolkitGamescopeInstanceListHandle]

        Library.toolkit().MaaToolkitGamescopeInstanceListAt.restype = MaaToolkitGamescopeInstanceHandle
        Library.toolkit().MaaToolkitGamescopeInstanceListAt.argtypes = [MaaToolkitGamescopeInstanceListHandle, MaaSize]

        Library.toolkit().MaaToolkitGamescopeInstanceGetDisplayNo.restype = ctypes.c_uint32
        Library.toolkit().MaaToolkitGamescopeInstanceGetDisplayNo.argtypes = [MaaToolkitGamescopeInstanceHandle]

        Library.toolkit().MaaToolkitGamescopeInstanceGetPipeWireNodeId.restype = ctypes.c_uint32
        Library.toolkit().MaaToolkitGamescopeInstanceGetPipeWireNodeId.argtypes = [MaaToolkitGamescopeInstanceHandle]

        Library.toolkit().MaaToolkitGamescopeInstanceGetEisSocketPath.restype = ctypes.c_char_p
        Library.toolkit().MaaToolkitGamescopeInstanceGetEisSocketPath.argtypes = [MaaToolkitGamescopeInstanceHandle]

        Library.toolkit().MaaToolkitDesktopWindowListCreate.restype = MaaToolkitDesktopWindowListHandle
        Library.toolkit().MaaToolkitDesktopWindowListCreate.argtypes = []

        Library.toolkit().MaaToolkitDesktopWindowListDestroy.restype = None
        Library.toolkit().MaaToolkitDesktopWindowListDestroy.argtypes = [MaaToolkitDesktopWindowListHandle]

        Library.toolkit().MaaToolkitDesktopWindowFindAll.restype = MaaBool
        Library.toolkit().MaaToolkitDesktopWindowFindAll.argtypes = [MaaToolkitDesktopWindowListHandle]

        Library.toolkit().MaaToolkitDesktopWindowListSize.restype = MaaSize
        Library.toolkit().MaaToolkitDesktopWindowListSize.argtypes = [MaaToolkitDesktopWindowListHandle]

        Library.toolkit().MaaToolkitDesktopWindowListAt.restype = MaaToolkitDesktopWindowHandle
        Library.toolkit().MaaToolkitDesktopWindowListAt.argtypes = [
            MaaToolkitDesktopWindowListHandle,
            MaaSize,
        ]

        Library.toolkit().MaaToolkitDesktopWindowGetHandle.restype = ctypes.c_void_p
        Library.toolkit().MaaToolkitDesktopWindowGetHandle.argtypes = [MaaToolkitDesktopWindowHandle]

        Library.toolkit().MaaToolkitDesktopWindowGetClassName.restype = ctypes.c_char_p
        Library.toolkit().MaaToolkitDesktopWindowGetClassName.argtypes = [MaaToolkitDesktopWindowHandle]

        Library.toolkit().MaaToolkitDesktopWindowGetWindowName.restype = ctypes.c_char_p
        Library.toolkit().MaaToolkitDesktopWindowGetWindowName.argtypes = [MaaToolkitDesktopWindowHandle]

        Library.toolkit().MaaToolkitMacOSCheckPermission.restype = MaaBool
        Library.toolkit().MaaToolkitMacOSCheckPermission.argtypes = [MaaMacOSPermission]

        Library.toolkit().MaaToolkitMacOSRequestPermission.restype = MaaBool
        Library.toolkit().MaaToolkitMacOSRequestPermission.argtypes = [MaaMacOSPermission]

        Library.toolkit().MaaToolkitMacOSRevealPermissionSettings.restype = MaaBool
        Library.toolkit().MaaToolkitMacOSRevealPermissionSettings.argtypes = [MaaMacOSPermission]

        Library.toolkit().MaaToolkitPortalHelperCreate.restype = MaaToolkitPortalHelperHandle
        Library.toolkit().MaaToolkitPortalHelperCreate.argtypes = []

        Library.toolkit().MaaToolkitPortalHelperDestroy.restype = None
        Library.toolkit().MaaToolkitPortalHelperDestroy.argtypes = [MaaToolkitPortalHelperHandle]

        Library.toolkit().MaaToolkitPortalHelperOpenStream.restype = MaaBool
        Library.toolkit().MaaToolkitPortalHelperOpenStream.argtypes = [MaaToolkitPortalHelperHandle]

        Library.toolkit().MaaToolkitPortalHelperGetPersist.restype = MaaBool
        Library.toolkit().MaaToolkitPortalHelperGetPersist.argtypes = [MaaToolkitPortalHelperHandle]

        Library.toolkit().MaaToolkitPortalHelperSetPersist.restype = None
        Library.toolkit().MaaToolkitPortalHelperSetPersist.argtypes = [MaaToolkitPortalHelperHandle, MaaBool]

        Library.toolkit().MaaToolkitPortalHelperGetPipeWireFD.restype = ctypes.c_int32
        Library.toolkit().MaaToolkitPortalHelperGetPipeWireFD.argtypes = [MaaToolkitPortalHelperHandle]

        Library.toolkit().MaaToolkitPortalHelperGetPipeWireNodeID.restype = ctypes.c_uint32
        Library.toolkit().MaaToolkitPortalHelperGetPipeWireNodeID.argtypes = [MaaToolkitPortalHelperHandle]

        Library.toolkit().MaaToolkitPortalHelperGetRestoreToken.restype = ctypes.c_char_p
        Library.toolkit().MaaToolkitPortalHelperGetRestoreToken.argtypes = [MaaToolkitPortalHelperHandle]

        Library.toolkit().MaaToolkitPortalHelperSetRestoreToken.restype = None
        Library.toolkit().MaaToolkitPortalHelperSetRestoreToken.argtypes = [MaaToolkitPortalHelperHandle, ctypes.c_char_p]

    _ios_api_properties_initialized: bool = False

    @staticmethod
    def _set_ios_api_properties() -> None:
        if Toolkit._ios_api_properties_initialized:
            return

        with Library._api_lock:
            if Toolkit._ios_api_properties_initialized:
                return

            try:
                Toolkit._assign_ios_api_properties()
            except AttributeError as error:
                raise RuntimeError(
                    "iOS Toolkit APIs are not available in the current build."
                ) from error
            Toolkit._ios_api_properties_initialized = True

    @staticmethod
    def _assign_ios_api_properties() -> None:
        api = Library.toolkit()
        api.MaaToolkitIOSDeviceListCreate.restype = ctypes.c_void_p
        api.MaaToolkitIOSDeviceListCreate.argtypes = []
        api.MaaToolkitIOSDeviceListDestroy.restype = None
        api.MaaToolkitIOSDeviceListDestroy.argtypes = [ctypes.c_void_p]
        api.MaaToolkitIOSDeviceFindWithOptions.restype = MaaBool
        api.MaaToolkitIOSDeviceFindWithOptions.argtypes = [MaaIOSConnectionMethod, ctypes.c_char_p,
            _IOSCancelCallback, ctypes.c_void_p, ctypes.c_void_p]
        api.MaaToolkitIOSDeviceListSize.restype = MaaSize
        api.MaaToolkitIOSDeviceListSize.argtypes = [ctypes.c_void_p]
        api.MaaToolkitIOSDeviceListAt.restype = ctypes.c_void_p
        api.MaaToolkitIOSDeviceListAt.argtypes = [ctypes.c_void_p, MaaSize]
        for name in ("MaaToolkitIOSDeviceGetName", "MaaToolkitIOSDeviceGetUdid",
                     "MaaToolkitIOSDeviceGetConnections", "MaaToolkitIOSDeviceGetState", "MaaToolkitIOSDeviceListGetWarnings"):
            function = getattr(api, name)
            function.restype = ctypes.c_char_p
            function.argtypes = [ctypes.c_void_p]
        api.MaaToolkitIOSDeviceGetScreencapMethods.restype = MaaIOSScreencapMethod
        api.MaaToolkitIOSDeviceGetScreencapMethods.argtypes = [ctypes.c_void_p]
