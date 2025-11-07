import json
from abc import abstractmethod
from ctypes import c_int32
import os
from pathlib import Path
from typing import Any, Dict, Optional, Union

from .buffer import ImageBuffer, StringBuffer
from .event_sink import EventSink, NotificationType
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
        """异步连接设备 / Asynchronously connect device

        这是一个异步操作，会立即返回一个 Job 对象
        This is an asynchronous operation that immediately returns a Job object

        Returns:
            Job: 作业对象，可通过 status/wait 查询状态 / Job object, can query status via status/wait
        """
        ctrl_id = Library.framework().MaaControllerPostConnection(self._handle)
        return self._gen_ctrl_job(ctrl_id)

    def post_click(self, x: int, y: int) -> Job:
        """异步点击 / Asynchronously click

        这是一个异步操作，会立即返回一个 Job 对象
        This is an asynchronous operation that immediately returns a Job object

        Args:
            x: x 坐标 / x coordinate
            y: y 坐标 / y coordinate

        Returns:
            Job: 作业对象，可通过 status/wait 查询状态 / Job object, can query status via status/wait
        """
        ctrl_id = Library.framework().MaaControllerPostClick(self._handle, x, y)
        return self._gen_ctrl_job(ctrl_id)

    def post_swipe(self, x1: int, y1: int, x2: int, y2: int, duration: int) -> Job:
        """滑动 / Swipe

        Args:
            x1: 起点 x 坐标 / Start x coordinate
            y1: 起点 y 坐标 / Start y coordinate
            x2: 终点 x 坐标 / End x coordinate
            y2: 终点 y 坐标 / End y coordinate
            duration: 滑动时长(毫秒) / Swipe duration in milliseconds

        Returns:
            Job: 作业对象 / Job object
        """
        ctrl_id = Library.framework().MaaControllerPostSwipe(
            self._handle, x1, y1, x2, y2, duration
        )
        return self._gen_ctrl_job(ctrl_id)

    def post_press_key(self, key: int) -> Job:
        """
        Deprecated: Use post_click_key instead.
        """
        return self.post_click_key(key)

    def post_click_key(self, key: int) -> Job:
        """单击按键 / Click key

        Args:
            key: 虚拟键码 / Virtual key code

        Returns:
            Job: 作业对象 / Job object
        """
        ctrl_id = Library.framework().MaaControllerPostClickKey(self._handle, key)
        return self._gen_ctrl_job(ctrl_id)

    def post_key_down(self, key: int) -> Job:
        """按下键 / Key down

        Args:
            key: 虚拟键码 / Virtual key code

        Returns:
            Job: 作业对象 / Job object
        """
        ctrl_id = Library.framework().MaaControllerPostKeyDown(self._handle, key)
        return self._gen_ctrl_job(ctrl_id)

    def post_key_up(self, key: int) -> Job:
        """抬起键 / Key up

        Args:
            key: 虚拟键码 / Virtual key code

        Returns:
            Job: 作业对象 / Job object
        """
        ctrl_id = Library.framework().MaaControllerPostKeyUp(self._handle, key)
        return self._gen_ctrl_job(ctrl_id)

    def post_input_text(self, text: str) -> Job:
        """输入文本 / Input text

        Args:
            text: 要输入的文本 / Text to input

        Returns:
            Job: 作业对象 / Job object
        """
        ctrl_id = Library.framework().MaaControllerPostInputText(
            self._handle, text.encode()
        )
        return self._gen_ctrl_job(ctrl_id)

    def post_start_app(self, intent: str) -> Job:
        """启动应用 / Start app

        Args:
            intent: 目标应用 (Adb 控制器: package name 或 activity) / Target app (Adb controller: package name or activity)

        Returns:
            Job: 作业对象 / Job object
        """
        ctrl_id = Library.framework().MaaControllerPostStartApp(
            self._handle, intent.encode()
        )
        return self._gen_ctrl_job(ctrl_id)

    def post_stop_app(self, intent: str) -> Job:
        """关闭应用 / Stop app

        Args:
            intent: 目标应用 (Adb 控制器: package name) / Target app (Adb controller: package name)

        Returns:
            Job: 作业对象 / Job object
        """
        ctrl_id = Library.framework().MaaControllerPostStopApp(
            self._handle, intent.encode()
        )
        return self._gen_ctrl_job(ctrl_id)

    def post_touch_down(
        self, x: int, y: int, contact: int = 0, pressure: int = 1
    ) -> Job:
        """按下 / Touch down

        Args:
            x: x 坐标 / x coordinate
            y: y 坐标 / y coordinate
            contact: 触点编号 (Adb 控制器: 手指编号; Win32 控制器: 鼠标按键 0:左键, 1:右键, 2:中键) / Contact number (Adb controller: finger number; Win32 controller: mouse button 0:left, 1:right, 2:middle)
            pressure: 触点力度 / Contact pressure

        Returns:
            Job: 作业对象 / Job object
        """
        ctrl_id = Library.framework().MaaControllerPostTouchDown(
            self._handle, contact, x, y, pressure
        )
        return self._gen_ctrl_job(ctrl_id)

    def post_touch_move(
        self, x: int, y: int, contact: int = 0, pressure: int = 1
    ) -> Job:
        """移动 / Move

        Args:
            x: x 坐标 / x coordinate
            y: y 坐标 / y coordinate
            contact: 触点编号 (Adb 控制器: 手指编号; Win32 控制器: 鼠标按键 0:左键, 1:右键, 2:中键) / Contact number (Adb controller: finger number; Win32 controller: mouse button 0:left, 1:right, 2:middle)
            pressure: 触点力度 / Contact pressure

        Returns:
            Job: 作业对象 / Job object
        """
        ctrl_id = Library.framework().MaaControllerPostTouchMove(
            self._handle, contact, x, y, pressure
        )
        return self._gen_ctrl_job(ctrl_id)

    def post_touch_up(self, contact: int = 0) -> Job:
        """抬起 / Touch up

        Args:
            contact: 触点编号 (Adb 控制器: 手指编号; Win32 控制器: 鼠标按键 0:左键, 1:右键, 2:中键) / Contact number (Adb controller: finger number; Win32 controller: mouse button 0:left, 1:right, 2:middle)

        Returns:
            Job: 作业对象 / Job object
        """
        ctrl_id = Library.framework().MaaControllerPostTouchUp(self._handle, contact)
        return self._gen_ctrl_job(ctrl_id)

    def post_screencap(self) -> JobWithResult:
        """截图 / Screenshot

        Returns:
            JobWithResult: 作业对象，可通过 result 获取截图 / Job object, can get screenshot via result
        """
        ctrl_id = Library.framework().MaaControllerPostScreencap(self._handle)
        return JobWithResult(
            ctrl_id,
            self._status,
            self._wait,
            self._get_screencap,
        )

    @property
    def cached_image(self) -> numpy.ndarray:
        """获取最新一次截图 / Get the latest screenshot

        Returns:
            numpy.ndarray: 截图图像 / Screenshot image

        Raises:
            RuntimeError: 如果获取失败
        """
        image_buffer = ImageBuffer()
        if not Library.framework().MaaControllerCachedImage(
            self._handle, image_buffer._handle
        ):
            raise RuntimeError("Failed to get cached image.")
        return image_buffer.get()

    @property
    def connected(self) -> bool:
        """判断是否已连接 / Check if connected

        Returns:
            bool: 是否已连接 / Whether connected
        """
        return bool(Library.framework().MaaControllerConnected(self._handle))

    @property
    def uuid(self) -> str:
        """获取设备 uuid / Get device uuid

        Returns:
            str: 设备 uuid / Device uuid

        Raises:
            RuntimeError: 如果获取失败
        """
        buffer = StringBuffer()
        if not Library.framework().MaaControllerGetUuid(self._handle, buffer._handle):
            raise RuntimeError("Failed to get UUID.")
        return buffer.get()

    def set_screenshot_target_long_side(self, long_side: int) -> bool:
        """设置截图缩放长边到指定长度 / Set screenshot scaling long side to specified length

        Args:
            long_side: 长边长度 / Long side length

        Returns:
            bool: 是否成功 / Whether successful
        """
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
        """设置截图缩放短边到指定长度 / Set screenshot scaling short side to specified length

        Args:
            short_side: 短边长度 / Short side length

        Returns:
            bool: 是否成功 / Whether successful
        """
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
        """设置截图不缩放 / Set screenshot use raw size without scaling

        注意：此选项可能导致在不同分辨率的设备上坐标不正确
        Note: This option may cause incorrect coordinates on devices with different resolutions

        Args:
            enable: 是否启用 / Whether to enable

        Returns:
            bool: 是否成功 / Whether successful
        """
        cbool = MaaBool(enable)
        return bool(
            Library.framework().MaaControllerSetOption(
                self._handle,
                MaaOption(MaaCtrlOptionEnum.ScreenshotUseRawSize),
                ctypes.pointer(cbool),
                ctypes.sizeof(MaaBool),
            )
        )

    _sink_holder: Dict[int, "ControllerEventSink"] = {}

    def add_sink(self, sink: "ControllerEventSink") -> Optional[int]:
        """添加控制器事件监听器 / Add controller event listener

        Args:
            sink: 事件监听器 / Event sink

        Returns:
            Optional[int]: 监听器 id，失败返回 None / Listener id, or None if failed
        """
        sink_id = int(
            Library.framework().MaaControllerAddSink(
                self._handle, *EventSink._gen_c_param(sink)
            )
        )
        if sink_id == MaaInvalidId:
            return None

        self._sink_holder[sink_id] = sink
        return sink_id

    def remove_sink(self, sink_id: int) -> None:
        """移除控制器事件监听器 / Remove controller event listener

        Args:
            sink_id: 监听器 id / Listener id
        """
        Library.framework().MaaControllerRemoveSink(self._handle, sink_id)
        self._sink_holder.pop(sink_id)

    def clear_sinks(self) -> None:
        """清除所有控制器事件监听器 / Clear all controller event listeners"""
        Library.framework().MaaControllerClearSinks(self._handle)

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

        Library.framework().MaaControllerPostClickKey.restype = MaaCtrlId
        Library.framework().MaaControllerPostClickKey.argtypes = [
            MaaControllerHandle,
            c_int32,
        ]

        Library.framework().MaaControllerPostKeyDown.restype = MaaCtrlId
        Library.framework().MaaControllerPostKeyDown.argtypes = [
            MaaControllerHandle,
            c_int32,
        ]
        Library.framework().MaaControllerPostKeyUp.restype = MaaCtrlId
        Library.framework().MaaControllerPostKeyUp.argtypes = [
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

        Library.framework().MaaControllerAddSink.restype = MaaSinkId
        Library.framework().MaaControllerAddSink.argtypes = [
            MaaControllerHandle,
            MaaEventCallback,
            ctypes.c_void_p,
        ]

        Library.framework().MaaControllerRemoveSink.restype = None
        Library.framework().MaaControllerRemoveSink.argtypes = [
            MaaControllerHandle,
            MaaSinkId,
        ]

        Library.framework().MaaControllerClearSinks.restype = None
        Library.framework().MaaControllerClearSinks.argtypes = [MaaControllerHandle]


class AdbController(Controller):
    """Adb 控制器 / Adb controller

    截图方式和输入方式会在启动时进行测速, 选择最快的方案
    Screenshot and input methods will be speed tested at startup, selecting the fastest option
    """
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
        notification_handler: None = None,
    ):
        """创建 Adb 控制器 / Create Adb controller

        Args:
            adb_path: adb 路径 / adb path
            address: 连接地址 / connection address
            screencap_methods: 所有可使用的截图方式 / all available screenshot methods
            input_methods: 所有可使用的输入方式 / all available input methods
            config: 额外配置 / extra config
            agent_path: MaaAgentBinary 路径 / MaaAgentBinary path
            notification_handler: 已废弃，请使用 add_sink 代替 / Deprecated, use add_sink instead

        Raises:
            NotImplementedError: 如果提供了 notification_handler
            RuntimeError: 如果创建失败
        """
        if notification_handler:
            raise NotImplementedError(
                "NotificationHandler is deprecated, use add_sink instead."
            )

        super().__init__()
        self._set_adb_api_properties()

        self._handle = Library.framework().MaaAdbControllerCreate(
            str(adb_path).encode(),
            address.encode(),
            MaaAdbScreencapMethod(screencap_methods),
            MaaAdbInputMethod(input_methods),
            json.dumps(config, ensure_ascii=False).encode(),
            str(agent_path).encode(),
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
        ]


class Win32Controller(Controller):
    """Win32 控制器 / Win32 controller"""

    def __init__(
        self,
        hWnd: Union[ctypes.c_void_p, int, None],
        screencap_method: int = MaaWin32ScreencapMethodEnum.DXGI_DesktopDup,
        mouse_method: int = MaaWin32InputMethodEnum.Seize,
        keyboard_method: int = MaaWin32InputMethodEnum.Seize,
        notification_handler: None = None,
    ):
        """创建 Win32 控制器 / Create Win32 controller

        Args:
            hWnd: 窗口句柄 / window handle
            screencap_method: 使用的截图方式 / screenshot method used
            mouse_method: 使用的鼠标输入方式 / mouse input method used
            keyboard_method: 使用的键盘输入方式 / keyboard input method used
            notification_handler: 已废弃，请使用 add_sink 代替 / Deprecated, use add_sink instead

        Raises:
            NotImplementedError: 如果提供了 notification_handler
            RuntimeError: 如果创建失败
        """
        if notification_handler:
            raise NotImplementedError(
                "NotificationHandler is deprecated, use add_sink instead."
            )

        super().__init__()
        self._set_win32_api_properties()

        self._handle = Library.framework().MaaWin32ControllerCreate(
            hWnd,
            MaaWin32ScreencapMethod(screencap_method),
            MaaWin32InputMethod(mouse_method),
            MaaWin32InputMethod(keyboard_method),
        )

        if not self._handle:
            raise RuntimeError("Failed to create Win32 controller.")

    def _set_win32_api_properties(self):
        Library.framework().MaaWin32ControllerCreate.restype = MaaControllerHandle
        Library.framework().MaaWin32ControllerCreate.argtypes = [
            ctypes.c_void_p,
            MaaWin32ScreencapMethod,
            MaaWin32InputMethod,
            MaaWin32InputMethod,
        ]


class DbgController(Controller):
    """调试控制器 / Debug controller"""

    def __init__(
        self,
        read_path: Union[str, Path],
        write_path: Union[str, Path],
        dbg_type: int,
        config: Dict[str, Any] = {},
        notification_handler: None = None,
    ):
        """创建调试控制器 / Create debug controller

        Args:
            read_path: 输入路径, 包含通过 Recording 选项记录的操作 / Input path, includes operations recorded via Recording option
            write_path: 输出路径, 包含执行结果 / Output path, includes execution results
            dbg_type: 控制器模式 / Controller mode
            config: 额外配置 / Extra config
            notification_handler: 已废弃，请使用 add_sink 代替 / Deprecated, use add_sink instead

        Raises:
            NotImplementedError: 如果提供了 notification_handler
            RuntimeError: 如果创建失败
        """
        if notification_handler:
            raise NotImplementedError(
                "NotificationHandler is deprecated, use add_sink instead."
            )

        super().__init__()
        self._set_dbg_api_properties()

        self._handle = Library.framework().MaaDbgControllerCreate(
            str(read_path).encode(),
            str(write_path).encode(),
            MaaDbgControllerType(dbg_type),
            json.dumps(config, ensure_ascii=False).encode(),
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
        ]


class CustomController(Controller):

    _callbacks: MaaCustomControllerCallbacks

    def __init__(
        self,
        notification_handler: None = None,
    ):
        if notification_handler:
            raise NotImplementedError(
                "NotificationHandler is deprecated, use add_sink instead."
            )

        super().__init__()
        self._set_custom_api_properties()

        self._callbacks = MaaCustomControllerCallbacks(
            CustomController._c_connect_agent,
            CustomController._c_request_uuid_agent,
            CustomController._c_get_features_agent,
            CustomController._c_start_app_agent,
            CustomController._c_stop_app_agent,
            CustomController._c_screencap_agent,
            CustomController._c_click_agent,
            CustomController._c_swipe_agent,
            CustomController._c_touch_down_agent,
            CustomController._c_touch_move_agent,
            CustomController._c_touch_up_agent,
            CustomController._c_click_key_agent,
            CustomController._c_input_text_agent,
            CustomController._c_key_down_agent,
            CustomController._c_key_up_agent,
        )

        self._handle = Library.framework().MaaCustomControllerCreate(
            self.c_handle,
            self.c_arg,
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
    
    def get_features(self) -> int:
        return MaaControllerFeatureEnum.UseMouseDownAndUpInsteadOfClick | MaaControllerFeatureEnum.UseKeyboardDownAndUpInsteadOfClick

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
    def click_key(self, keycode: int) -> bool:
        raise NotImplementedError

    @abstractmethod
    def input_text(self, text: str) -> bool:
        raise NotImplementedError

    @abstractmethod
    def key_down(self, keycode: int) -> bool:
        raise NotImplementedError

    @abstractmethod
    def key_up(self, keycode: int) -> bool:
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
    @MaaCustomControllerCallbacks.GetFeaturesFunc
    def _c_get_features_agent(trans_arg: ctypes.c_void_p) -> int:
        if not trans_arg:
            return int(MaaControllerFeatureEnum.Null)

        self: CustomController = ctypes.cast(
            trans_arg,
            ctypes.py_object,
        ).value

        return int(self.get_features())

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
    @MaaCustomControllerCallbacks.ClickKeyFunc
    def _c_click_key_agent(
        c_keycode: ctypes.c_int32,
        trans_arg: ctypes.c_void_p,
    ) -> int:
        if not trans_arg:
            return int(False)

        self: CustomController = ctypes.cast(
            trans_arg,
            ctypes.py_object,
        ).value

        return int(self.click_key(int(c_keycode)))

    @staticmethod
    @MaaCustomControllerCallbacks.KeyDownFunc
    def _c_key_down_agent(
        c_keycode: ctypes.c_int32,
        trans_arg: ctypes.c_void_p,
    ) -> int:
        if not trans_arg:
            return int(False)

        self: CustomController = ctypes.cast(
            trans_arg,
            ctypes.py_object,
        ).value

        return int(self.key_down(int(c_keycode)))

    @staticmethod
    @MaaCustomControllerCallbacks.KeyUpFunc
    def _c_key_up_agent(
        c_keycode: ctypes.c_int32,
        trans_arg: ctypes.c_void_p,
    ) -> int:
        if not trans_arg:
            return int(False)

        self: CustomController = ctypes.cast(
            trans_arg,
            ctypes.py_object,
        ).value

        return int(self.key_up(int(c_keycode)))

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
        ]


class ControllerEventSink(EventSink):

    @dataclass
    class ControllerActionDetail:
        ctrl_id: int
        uuid: str
        action: str
        param: dict

    def on_controller_action(
        self,
        controller: Controller,
        noti_type: NotificationType,
        detail: ControllerActionDetail,
    ):
        pass

    def on_raw_notification(self, controller: Controller, msg: str, details: dict):
        pass

    def _on_raw_notification(self, handle: ctypes.c_void_p, msg: str, details: dict):

        controller = Controller(handle=handle)
        self.on_raw_notification(controller, msg, details)

        noti_type = EventSink._notification_type(msg)
        if msg.startswith("Controller.Action"):
            detail = self.ControllerActionDetail(
                ctrl_id=details["ctrl_id"],
                uuid=details["uuid"],
                action=details["action"],
                param=details["param"],
            )
            self.on_controller_action(controller, noti_type, detail)

        else:
            self.on_unknown_notification(controller, msg, details)
