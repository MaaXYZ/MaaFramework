import ctypes
import json
from abc import ABC
from typing import Optional, Any, Dict

from .define import MaaApiCallback, MaaBool, MaaId, MaaStatus
from .common import Status
from .library import Library
from .callback_agent import CallbackAgent, Callback


class Controller(ABC):
    _handle: ctypes.c_void_p
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

    def connect(self) -> bool:
        """
        Sync connect to the controller.

        :return: True if the connection was successful, False otherwise.
        """

        cid = self.post_connection()
        return self.wait(cid) == Status.success

    def post_connection(self) -> int:
        """
        Async post a connection to the controller.

        :return: The connection ID.
        """

        return Library.framework.MaaControllerPostConnection(self._handle)

    def status(self, connection_id: int) -> Status:
        """
        Get the status of a connection.

        :param connection_id: The connection ID.
        :return: The status of the connection.
        """

        return Status(
            Library.framework.MaaControllerStatus(self._handle, connection_id)
        )

    def wait(self, connection_id: int) -> Status:
        """
        Wait for a connection to complete.

        :param connection_id: The connection ID.
        :return: The status of the connection.
        """

        return Status(Library.framework.MaaControllerWait(self._handle, connection_id))

    def connected(self) -> bool:
        """
        Check if the controller is connected.

        :return: True if the controller is connected, False otherwise.
        """

        return bool(Library.framework.MaaControllerConnected(self._handle))

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
        Library.framework.MaaControllerDestroy.argtypes = [ctypes.c_void_p]

        Library.framework.MaaControllerSetOption.restype = MaaBool
        Library.framework.MaaControllerSetOption.argtypes = [
            ctypes.c_void_p,
            ctypes.c_int32,
            ctypes.c_void_p,
            ctypes.c_uint64,
        ]

        Library.framework.MaaControllerPostConnection.restype = MaaId
        Library.framework.MaaControllerPostConnection.argtypes = [ctypes.c_void_p]

        Library.framework.MaaControllerStatus.restype = MaaStatus
        Library.framework.MaaControllerStatus.argtypes = [ctypes.c_void_p, MaaId]

        Library.framework.MaaControllerWait.restype = MaaStatus
        Library.framework.MaaControllerWait.argtypes = [ctypes.c_void_p, MaaId]

        Library.framework.MaaControllerConnected.restype = MaaBool
        Library.framework.MaaControllerConnected.argtypes = [ctypes.c_void_p]


class AdbController(Controller):
    DEFAULT_CONFIG = {
        "prebuilt": {
            "minicap": {
                "arch": ["x86", "armeabi-v7a", "armeabi"],
                "sdk": [31, 29, 28, 27, 26, 25, 24, 23, 22, 21, 19, 18, 17, 16, 15, 14],
            },
            "minitouch": {
                "arch": ["x86_64", "x86", "arm64-v8a", "armeabi-v7a", "armeabi"],
            },
            "maatouch": {
                "package": "com.shxyke.MaaTouch.App",
            },
        },
        "command": {
            "Devices": ["{ADB}", "devices"],
            "Connect": ["{ADB}", "connect", "{ADB_SERIAL}"],
            "KillServer": ["{ADB}", "kill-server"],
            "UUID": [
                "{ADB}",
                "-s",
                "{ADB_SERIAL}",
                "shell",
                "settings get secure android_id",
            ],
            "Resolution": [
                "{ADB}",
                "-s",
                "{ADB_SERIAL}",
                "shell",
                "dumpsys window displays | grep -o -E cur=+[^\\ ]+ | grep -o -E [0-9]+",
            ],
            "StartApp": [
                "{ADB}",
                "-s",
                "{ADB_SERIAL}",
                "shell",
                "am start -n {INTENT}",
            ],
            "StopApp": [
                "{ADB}",
                "-s",
                "{ADB_SERIAL}",
                "shell",
                "am force-stop {INTENT}",
            ],
            "Click": ["{ADB}", "-s", "{ADB_SERIAL}", "shell", "input tap {X} {Y}"],
            "Swipe": [
                "{ADB}",
                "-s",
                "{ADB_SERIAL}",
                "shell",
                "input swipe {X1} {Y1} {X2} {Y2} {DURATION}",
            ],
            "PressKey": [
                "{ADB}",
                "-s",
                "{ADB_SERIAL}",
                "shell",
                "input keyevent {KEY}",
            ],
            "ForwardSocket": [
                "{ADB}",
                "-s",
                "{ADB_SERIAL}",
                "forward",
                "tcp:{FOWARD_PORT}",
                "localabstract:{LOCAL_SOCKET}",
            ],
            "NetcatAddress": [
                "{ADB}",
                "-s",
                "{ADB_SERIAL}",
                "shell",
                "cat /proc/net/arp | grep : ",
            ],
            "ScreencapRawByNetcat": [
                "{ADB}",
                "-s",
                "{ADB_SERIAL}",
                "exec-out",
                "screencap | nc -w 3 {NETCAT_ADDRESS} {NETCAT_PORT}",
            ],
            "ScreencapRawWithGzip": [
                "{ADB}",
                "-s",
                "{ADB_SERIAL}",
                "exec-out",
                "screencap | gzip -1",
            ],
            "ScreencapEncode": [
                "{ADB}",
                "-s",
                "{ADB_SERIAL}",
                "exec-out",
                "screencap -p",
            ],
            "ScreencapEncodeToFile": [
                "{ADB}",
                "-s",
                "{ADB_SERIAL}",
                "shell",
                'screencap -p > "/data/local/tmp/{TEMP_FILE}"',
            ],
            "PullFile": [
                "{ADB}",
                "-s",
                "{ADB_SERIAL}",
                "pull",
                "/data/local/tmp/{TEMP_FILE}",
                "{DST_PATH}",
            ],
            "Abilist": [
                "{ADB}",
                "-s",
                "{ADB_SERIAL}",
                "shell",
                "getprop ro.product.cpu.abilist | tr -d '\n\r'",
            ],
            "SDK": [
                "{ADB}",
                "-s",
                "{ADB_SERIAL}",
                "shell",
                "getprop ro.build.version.sdk | tr -d '\n\r'",
            ],
            "Orientation": [
                "{ADB}",
                "-s",
                "{ADB_SERIAL}",
                "shell",
                "dumpsys input | grep SurfaceOrientation | grep -m 1 -o -E [0-9]",
            ],
            "PushBin": [
                "{ADB}",
                "-s",
                "{ADB_SERIAL}",
                "push",
                "{BIN_PATH}",
                "/data/local/tmp/{BIN_WORKING_FILE}",
            ],
            "ChmodBin": [
                "{ADB}",
                "-s",
                "{ADB_SERIAL}",
                "shell",
                'chmod 700 "/data/local/tmp/{BIN_WORKING_FILE}"',
            ],
            "InvokeBin": [
                "{ADB}",
                "-s",
                "{ADB_SERIAL}",
                "shell",
                'export LD_LIBRARY_PATH=/data/local/tmp/; "/data/local/tmp/{BIN_WORKING_FILE}" {BIN_EXTRA_PARAMS}',
            ],
            "InvokeApp": [
                "{ADB}",
                "-s",
                "{ADB_SERIAL}",
                "shell",
                'export CLASSPATH="/data/local/tmp/{APP_WORKING_FILE}"; app_process /data/local/tmp {PACKAGE_NAME}',
            ],
        },
    }

    def __init__(
        self,
        adb_path: str,
        address: str,
        controller_type: int = 65793,
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

        Library.framework.MaaAdbControllerCreateV2.restype = ctypes.c_void_p
        Library.framework.MaaAdbControllerCreateV2.argtypes = [
            ctypes.c_char_p,
            ctypes.c_char_p,
            ctypes.c_int32,
            ctypes.c_char_p,
            ctypes.c_char_p,
            MaaApiCallback,
            ctypes.c_void_p,
        ]
