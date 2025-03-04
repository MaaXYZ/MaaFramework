import ctypes
import ctypes.util
import pathlib
import platform

from .define import *


class Library:
    _is_agent_server: bool = False

    @staticmethod
    def open(path: pathlib.Path, agent_server: bool = False):
        if not path.exists():
            raise FileNotFoundError(f"`{path}` does not exist.")

        WINDOWS = "windows"
        MACOS = "darwin"
        LINUX = "linux"

        Library._is_agent_server = agent_server

        if not agent_server:
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
            framework_library = {
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
            lib_import = ctypes.WinDLL
        else:
            lib_import = ctypes.CDLL

        Library.framework_libpath = path / framework_library[platform_type]
        Library.framework = lib_import(str(Library.framework_libpath))

        Library.toolkit_libpath = path / toolkit_library[platform_type]
        Library.toolkit = lib_import(str(Library.toolkit_libpath))

        if not agent_server:
            Library.agent_client_libpath = path / agent_client_library[platform_type]
            Library.agent_client = lib_import(str(Library.agent_client_libpath))
        else:
            Library.agent_server_libpath = Library.framework_libpath
            Library.agent_server = Library.framework

        Library._set_api_properties()

    @staticmethod
    def is_agent_server() -> bool:
        return Library._is_agent_server

    @staticmethod
    def version() -> str:
        return Library.framework.MaaVersion().decode()

    @staticmethod
    def _set_api_properties():
        Library.framework.MaaVersion.restype = ctypes.c_char_p
        Library.framework.MaaVersion.argtypes = []
