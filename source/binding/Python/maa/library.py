import ctypes
import ctypes.util
import pathlib
import platform

from .define import *


class Library:
    _is_agent_server: bool = False

    _framework: ctypes.CDLL = None
    _toolkit: ctypes.CDLL = None
    _agent_client: ctypes.CDLL = None
    _agent_server: ctypes.CDLL = None
    _lib_type = None

    @staticmethod
    def open(path: pathlib.Path, agent_server: bool = False):
        if Library._api_properties_initialized:
            return

        if not path.exists():
            raise FileNotFoundError(f"`{path}` does not exist.")

        WINDOWS = "windows"
        MACOS = "darwin"
        LINUX = "linux"

        Library._is_agent_server = agent_server

        if not Library.is_agent_server():
            framework_library = {
                WINDOWS: "MaaFramework.dll",
                MACOS: "libMaaFramework.dylib",
                LINUX: "libMaaFramework.so",
            }
            agent_client_library = {
                WINDOWS: "MaaAgentClient.dll",
                MACOS: "libMaaAgentClient.dylib",
                LINUX: "libMaaAgentClient.so",
            }
        else:
            agent_server_library = {
                WINDOWS: "MaaAgentServer.dll",
                MACOS: "libMaaAgentServer.dylib",
                LINUX: "libMaaAgentServer.so",
            }

        toolkit_library = {
            WINDOWS: "MaaToolkit.dll",
            MACOS: "libMaaToolkit.dylib",
            LINUX: "libMaaToolkit.so",
        }

        platform_type = platform.system().lower()

        if platform_type == "windows":
            Library._lib_type = ctypes.WinDLL
        else:
            Library._lib_type = ctypes.CDLL

        if not Library.is_agent_server():
            Library.framework_libpath = path / framework_library[platform_type]
            Library.agent_client_libpath = path / agent_client_library[platform_type]
        else:
            Library.agent_server_libpath = path / agent_server_library[platform_type]

        Library.toolkit_libpath = path / toolkit_library[platform_type]

    @staticmethod
    def framework() -> ctypes.CDLL:
        if not Library.is_agent_server():
            if not Library._framework:
                Library._framework = Library._lib_type(str(Library.framework_libpath))

            return Library._framework
        else:
            return Library.agent_server()

    @staticmethod
    def toolkit() -> ctypes.CDLL:
        if not Library._toolkit:
            Library._toolkit = Library._lib_type(str(Library.toolkit_libpath))

        return Library._toolkit

    @staticmethod
    def agent_client() -> ctypes.CDLL:
        if Library.is_agent_server():
            raise ValueError("Agent server is not available in the current context.")

        if not Library._agent_client:
            Library._agent_client = Library._lib_type(
                str(Library.agent_client_libpath)
            )

        return Library._agent_client

    @staticmethod
    def agent_server() -> ctypes.CDLL:
        if not Library.is_agent_server():
            raise ValueError("Agent client is not available in the current context.")

        if not Library._agent_server:
            Library._agent_server = Library._lib_type(
                str(Library.agent_server_libpath)
            )

        return Library._agent_server

    @staticmethod
    def is_agent_server() -> bool:
        return Library._is_agent_server

    @staticmethod
    def version() -> str:
        Library._set_api_properties()

        return Library.framework().MaaVersion().decode()

    _api_properties_initialized: bool = False

    @staticmethod
    def _set_api_properties():
        if Library._api_properties_initialized:
            return

        Library._api_properties_initialized = True

        Library.framework().MaaVersion.restype = ctypes.c_char_p
        Library.framework().MaaVersion.argtypes = []
