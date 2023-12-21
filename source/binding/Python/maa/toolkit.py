import ctypes
import json
import asyncio
from dataclasses import dataclass

from .library import Library
from .define import MaaBool
from .instance import Instance


class Toolkit:
    @classmethod
    def init_config(cls) -> bool:
        """
        Init the toolkit config.
        """
        cls._set_api_properties()

        return Library.toolkit.MaaToolKitInit()

    @classmethod
    async def adb_devices(cls) -> list:
        """
        Get the adb devices.
        """
        cls._set_api_properties()

        Library.toolkit.MaaToolKitPostFindDevice()

        while not Library.toolkit.MaaToolKitIsFindDeviceCompleted():
            await asyncio.sleep(0)

        count = Library.toolkit.MaaToolKitGetDeviceCount()
        devices = []
        for i in range(count):
            name = Library.toolkit.MaaToolKitGetDeviceName(i).decode("utf-8")
            adb_path = Library.toolkit.MaaToolKitGetDeviceAdbPath(i).decode("utf-8")
            address = Library.toolkit.MaaToolKitGetDeviceAdbSerial(i).decode("utf-8")
            controller_type = int(
                Library.toolkit.MaaToolKitGetDeviceAdbControllerType(i)
            )
            config = Library.toolkit.MaaToolKitGetDeviceAdbConfig(i).decode("utf-8")

            devices.append(AdbDevice(name, adb_path, address, controller_type, config))

        return devices

    @classmethod
    def register_recognizer_exec_agent(
        cls, inst: Instance, name: str, exec_path: str, argv: list
    ) -> bool:
        """
        Register a recognizer exec agent.
        """
        cls._set_api_properties()

        json_argv = json.dumps(argv)
        return Library.toolkit.MaaToolKitRegisterCustomRecognizerExecutor(
            inst.c_handle(),
            name.encode("utf-8"),
            exec_path.encode("utf-8"),
            json_argv.encode("utf-8"),
        )

    @classmethod
    def register_action_exec_agent(
        cls, inst: Instance, name: str, exec_path: str, argv: list
    ) -> bool:
        """
        Register a action exec agent.
        """
        cls._set_api_properties()

        json_argv = json.dumps(argv)
        return Library.toolkit.MaaToolKitRegisterCustomActionExecutor(
            inst.c_handle(),
            name.encode("utf-8"),
            exec_path.encode("utf-8"),
            json_argv.encode("utf-8"),
        )

    _api_properties_initialized: bool = False

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

        Library.toolkit.MaaToolKitInit.restype = MaaBool
        Library.toolkit.MaaToolKitInit.argtypes = None

        Library.toolkit.MaaToolKitUninit.restype = MaaBool
        Library.toolkit.MaaToolKitUninit.argtypes = None

        Library.toolkit.MaaToolKitPostFindDevice.restype = MaaBool
        Library.toolkit.MaaToolKitPostFindDevice.argtypes = None

        Library.toolkit.MaaToolKitIsFindDeviceCompleted.restype = MaaBool
        Library.toolkit.MaaToolKitIsFindDeviceCompleted.argtypes = None

        Library.toolkit.MaaToolKitGetDeviceCount.restype = ctypes.c_size_t
        Library.toolkit.MaaToolKitGetDeviceCount.argtypes = None

        Library.toolkit.MaaToolKitGetDeviceName.restype = ctypes.c_char_p
        Library.toolkit.MaaToolKitGetDeviceName.argtypes = [ctypes.c_size_t]

        Library.toolkit.MaaToolKitGetDeviceAdbPath.restype = ctypes.c_char_p
        Library.toolkit.MaaToolKitGetDeviceAdbPath.argtypes = [ctypes.c_size_t]

        Library.toolkit.MaaToolKitGetDeviceAdbSerial.restype = ctypes.c_char_p
        Library.toolkit.MaaToolKitGetDeviceAdbSerial.argtypes = [ctypes.c_size_t]

        Library.toolkit.MaaToolKitGetDeviceAdbControllerType.restype = ctypes.c_int32
        Library.toolkit.MaaToolKitGetDeviceAdbControllerType.argtypes = [
            ctypes.c_size_t
        ]

        Library.toolkit.MaaToolKitGetDeviceAdbConfig.restype = ctypes.c_char_p
        Library.toolkit.MaaToolKitGetDeviceAdbConfig.argtypes = [ctypes.c_size_t]

        Library.toolkit.MaaToolKitRegisterCustomRecognizerExecutor.restype = MaaBool
        Library.toolkit.MaaToolKitRegisterCustomRecognizerExecutor.argtypes = [
            ctypes.c_void_p,
            ctypes.c_char_p,
            ctypes.c_char_p,
            ctypes.c_char_p,
        ]

        Library.toolkit.MaaToolKitRegisterCustomActionExecutor.restype = MaaBool
        Library.toolkit.MaaToolKitRegisterCustomActionExecutor.argtypes = [
            ctypes.c_void_p,
            ctypes.c_char_p,
            ctypes.c_char_p,
            ctypes.c_char_p,
        ]


@dataclass
class AdbDevice:
    name: str
    adb_path: str
    address: str
    controller_type: int
    config: str
