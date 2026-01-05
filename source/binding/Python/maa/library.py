import ctypes
import pathlib
import platform
from typing import Optional

from .define import *


class Library:
    """库加载管理器 / Library loading manager

    管理 MaaFramework 各动态库的加载和访问。
    Manages loading and access to MaaFramework dynamic libraries.
    """

    _is_agent_server: bool = False

    _framework: Optional[ctypes.CDLL] = None
    _toolkit: Optional[ctypes.CDLL] = None
    _agent_client: Optional[ctypes.CDLL] = None
    _agent_server: Optional[ctypes.CDLL] = None
    _lib_type = None

    @staticmethod
    def open(path: pathlib.Path, agent_server: bool = False):
        """打开并加载库 / Open and load libraries

        Args:
            path: 库文件所在目录 / Directory containing library files
            agent_server: 是否以 AgentServer 模式加载 / Whether to load in AgentServer mode

        Raises:
            FileNotFoundError: 如果路径不存在
        """
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

        if platform_type == WINDOWS:
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
        """获取 MaaFramework 库 / Get MaaFramework library

        Returns:
            ctypes.CDLL: MaaFramework 动态库对象 / MaaFramework dynamic library object
        """
        if not Library.is_agent_server():
            if not Library._framework:
                Library._framework = Library._lib_type(str(Library.framework_libpath))  # type: ignore

            return Library._framework
        else:
            return Library.agent_server()

    @staticmethod
    def toolkit() -> ctypes.CDLL:
        """获取 MaaToolkit 库 / Get MaaToolkit library

        Returns:
            ctypes.CDLL: MaaToolkit 动态库对象 / MaaToolkit dynamic library object
        """
        if not Library._toolkit:
            Library._toolkit = Library._lib_type(str(Library.toolkit_libpath))  # type: ignore

        return Library._toolkit

    @staticmethod
    def agent_client() -> ctypes.CDLL:
        """获取 MaaAgentClient 库 / Get MaaAgentClient library

        Returns:
            ctypes.CDLL: MaaAgentClient 动态库对象 / MaaAgentClient dynamic library object

        Raises:
            ValueError: 如果在 AgentServer 模式下调用
        """
        if Library.is_agent_server():
            raise ValueError("Agent server is not available in the current context.")

        if not Library._agent_client:
            Library._agent_client = Library._lib_type(str(Library.agent_client_libpath))  # type: ignore

        return Library._agent_client

    @staticmethod
    def agent_server() -> ctypes.CDLL:
        """获取 MaaAgentServer 库 / Get MaaAgentServer library

        Returns:
            ctypes.CDLL: MaaAgentServer 动态库对象 / MaaAgentServer dynamic library object

        Raises:
            ValueError: 如果不在 AgentServer 模式下调用
        """
        if not Library.is_agent_server():
            raise ValueError("Agent client is not available in the current context.")

        if not Library._agent_server:
            Library._agent_server = Library._lib_type(str(Library.agent_server_libpath))  # type: ignore

        return Library._agent_server

    @staticmethod
    def is_agent_server() -> bool:
        """判断是否为 AgentServer 模式 / Check if in AgentServer mode

        Returns:
            bool: 是否为 AgentServer 模式 / Whether in AgentServer mode
        """
        return Library._is_agent_server

    @staticmethod
    def version() -> str:
        """获取 MaaFramework 版本 / Get MaaFramework version

        Returns:
            str: 版本字符串 / Version string
        """
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
