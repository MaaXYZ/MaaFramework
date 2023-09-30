import ctypes
from abc import ABC
from typing import Callable, Any

from .define import maa_api_callback, maa_bool, maa_id, maa_status
from .library import library


class controller(ABC):
    def __init__(self):
        """
        Base class for all controllers.
        """

        if not library.initialized:
            raise RuntimeError("Library not initialized, please call `library.open()` first.")
        
        self._set_api_properties()
        self._handle = None


    def __del__(self):
        """
        Destructor for the controller.
        """
        if self._handle is not None:
            library.framework.MaaControllerDestroy(self._handle)
            self._handle = None


    def connect(self) -> bool:
        """
        Sync connect to the controller.

        :return: True if the connection was successful, False otherwise.
        """

        cid = self.post_connect()
        return self.wait(cid) == maa_status.success


    def post_connect(self) -> int:
        """
        Async post a connection to the controller.

        :return: The connection ID.
        """
        
        return library.framework.MaaControllerPostConnection(self._handle)
    

    def status(self, connection_id: int) -> maa_status:
        """
        Get the status of a connection.

        :param connection_id: The connection ID.
        :return: The status of the connection.
        """

        return maa_status(library.framework.MaaControllerStatus(self._handle, connection_id))


    def wait(self, connection_id: int) -> maa_status:
        """
        Wait for a connection to complete.

        :param connection_id: The connection ID.
        :return: The status of the connection.
        """

        return maa_status(library.framework.MaaControllerWait(self._handle, connection_id))
    

    def connected(self) -> bool:
        """
        Check if the controller is connected.

        :return: True if the controller is connected, False otherwise.
        """

        ret = library.framework.MaaControllerConnected(self._handle)
        return True if ret else False


    def _set_api_properties(self):
        """
        Set the API properties for the controller.
        """

        library.framework.MaaControllerDestroy.restype = None
        library.framework.MaaControllerDestroy.argtypes = [ctypes.c_void_p]

        library.framework.MaaControllerSetOption.restype = maa_bool
        library.framework.MaaControllerSetOption.argtypes = [ctypes.c_void_p, ctypes.c_int32, ctypes.c_void_p, ctypes.c_uint64]

        library.framework.MaaControllerPostConnection.restype = maa_id
        library.framework.MaaControllerPostConnection.argtypes = [ctypes.c_void_p]

        library.framework.MaaControllerStatus.restype = ctypes.c_int32
        library.framework.MaaControllerStatus.argtypes = [ctypes.c_void_p, maa_id]

        library.framework.MaaControllerWait.restype = ctypes.c_int32
        library.framework.MaaControllerWait.argtypes = [ctypes.c_void_p, maa_id]
        
        library.framework.MaaControllerConnected.restype = maa_bool
        library.framework.MaaControllerConnected.argtypes = [ctypes.c_void_p]


    def _controller_callback(msg: ctypes.c_char_p, details_json: ctypes.c_char_p, callback_arg: ctypes.c_void_p):
        if not callback_arg:
            return
        
        self: controller = ctypes.cast(callback_arg, ctypes.py_object).value
        if not self._callback:
            return
        
        self._callback(msg.decode('utf-8'), details_json.decode('utf-8'), self._callback_arg)

    def _self_to_void_p(self):
        return ctypes.c_void_p.from_buffer(ctypes.py_object(self))


class adb_controller(controller):
    default_config = {
        "prebuilt": {
            "minicap": {
                "root": "./MaaAgentBinary/minicap",
                "arch": [
                    "x86",
                    "armeabi-v7a",
                    "armeabi"
                ],
                "sdk": [
                    31, 29, 28, 27, 26, 25, 24, 23, 22, 21, 19, 18, 17, 16, 15, 14
                ]
            },
            "minitouch": {
                "root": "./MaaAgentBinary/minitouch",
                "arch": [
                    "x86_64",
                    "x86",
                    "arm64-v8a",
                    "armeabi-v7a",
                    "armeabi"
                ]
            },
            "maatouch": {
                "root": "./MaaAgentBinary/maatouch",
                "package": "com.shxyke.MaaTouch.App"
            }
        },
        "command": {
            "Devices": [
                "{ADB}",
                "devices"
            ],
            "Connect": [
                "{ADB}",
                "connect",
                "{ADB_SERIAL}"
            ],
            "KillServer": [
                "{ADB}",
                "kill-server"
            ],
            "UUID": [
                "{ADB}",
                "-s",
                "{ADB_SERIAL}",
                "shell",
                "settings get secure android_id"
            ],
            "Resolution": [
                "{ADB}",
                "-s",
                "{ADB_SERIAL}",
                "shell",
                "dumpsys window displays | grep -o -E cur=+[^\\ ]+ | grep -o -E [0-9]+"
            ],
            "StartApp": [
                "{ADB}",
                "-s",
                "{ADB_SERIAL}",
                "shell",
                "am start -n {INTENT}"
            ],
            "StopApp": [
                "{ADB}",
                "-s",
                "{ADB_SERIAL}",
                "shell",
                "am force-stop {INTENT}"
            ],
            "Click": [
                "{ADB}",
                "-s",
                "{ADB_SERIAL}",
                "shell",
                "input tap {X} {Y}"
            ],
            "Swipe": [
                "{ADB}",
                "-s",
                "{ADB_SERIAL}",
                "shell",
                "input swipe {X1} {Y1} {X2} {Y2} {DURATION}"
            ],
            "PressKey": [
                "{ADB}",
                "-s",
                "{ADB_SERIAL}",
                "shell",
                "input keyevent {KEY}"
            ],
            "ForwardSocket": [
                "{ADB}",
                "-s",
                "{ADB_SERIAL}",
                "forward",
                "tcp:{FOWARD_PORT}",
                "localabstract:{LOCAL_SOCKET}"
            ],
            "NetcatAddress": [
                "{ADB}",
                "-s",
                "{ADB_SERIAL}",
                "shell",
                "cat /proc/net/arp | grep : "
            ],
            "ScreencapRawByNetcat": [
                "{ADB}",
                "-s",
                "{ADB_SERIAL}",
                "exec-out",
                "screencap | nc -w 3 {NETCAT_ADDRESS} {NETCAT_PORT}"
            ],
            "ScreencapRawWithGzip": [
                "{ADB}",
                "-s",
                "{ADB_SERIAL}",
                "exec-out",
                "screencap | gzip -1"
            ],
            "ScreencapEncode": [
                "{ADB}",
                "-s",
                "{ADB_SERIAL}",
                "exec-out",
                "screencap -p"
            ],
            "ScreencapEncodeToFile": [
                "{ADB}",
                "-s",
                "{ADB_SERIAL}",
                "shell",
                "screencap -p > \"/data/local/tmp/{TEMP_FILE}\""
            ],
            "PullFile": [
                "{ADB}",
                "-s",
                "{ADB_SERIAL}",
                "pull",
                "/data/local/tmp/{TEMP_FILE}",
                "{DST_PATH}"
            ],
            "Abilist": [
                "{ADB}",
                "-s",
                "{ADB_SERIAL}",
                "shell",
                "getprop ro.product.cpu.abilist | tr -d '\n\r'"
            ],
            "SDK": [
                "{ADB}",
                "-s",
                "{ADB_SERIAL}",
                "shell",
                "getprop ro.build.version.sdk | tr -d '\n\r'"
            ],
            "Orientation": [
                "{ADB}",
                "-s",
                "{ADB_SERIAL}",
                "shell",
                "dumpsys input | grep SurfaceOrientation | grep -m 1 -o -E [0-9]"
            ],
            "PushBin": [
                "{ADB}",
                "-s",
                "{ADB_SERIAL}",
                "push",
                "{BIN_PATH}",
                "/data/local/tmp/{BIN_WORKING_FILE}"
            ],
            "ChmodBin": [
                "{ADB}",
                "-s",
                "{ADB_SERIAL}",
                "shell",
                "chmod 700 \"/data/local/tmp/{BIN_WORKING_FILE}\""
            ],
            "InvokeBin": [
                "{ADB}",
                "-s",
                "{ADB_SERIAL}",
                "shell",
                "export LD_LIBRARY_PATH=/data/local/tmp/; \"/data/local/tmp/{BIN_WORKING_FILE}\" {BIN_EXTRA_PARAMS}"
            ],
            "InvokeApp": [
                "{ADB}",
                "-s",
                "{ADB_SERIAL}",
                "shell",
                "export CLASSPATH=\"/data/local/tmp/{APP_WORKING_FILE}\"; app_process /data/local/tmp {PACKAGE_NAME}"
            ]
        }
    }

    def __init__(self, adb_path: str, address: str, controller_type: int = 65793, config: str = default_config,
                 callback: Callable = None, callback_arg: Any = None):
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

        self._callback = callback
        self._callback_arg = callback_arg

        self._handle = library.framework.MaaAdbControllerCreate(
            adb_path.encode('utf-8'),
            address.encode('utf-8'),
            controller_type,
            config.encode('utf-8'),
            self._controller_callback,
            self._self_to_void_p(),
        )


    def _set_adb_api_properties(self):
        """
        Set the API properties for the ADB controller.
        """

        library.framework.MaaAdbControllerCreate.restype = ctypes.c_void_p
        library.framework.MaaAdbControllerCreate.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_int32, ctypes.c_char_p, maa_api_callback, ctypes.c_void_p]
