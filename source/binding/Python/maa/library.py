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

    @classmethod
    def open(cls, path: pathlib.Path, agent_server: bool = False):
        """打开并加载库 / Open and load libraries

        Args:
            path: 库文件所在目录 / Directory containing library files
            agent_server: 是否以 AgentServer 模式加载 / Whether to load in AgentServer mode

        Raises:
            FileNotFoundError: 如果路径不存在
        """
        if cls._api_properties_initialized:
            return

        if not path.exists():
            raise FileNotFoundError(f"`{path}` does not exist.")

        WINDOWS = "windows"
        MACOS = "darwin"
        LINUX = "linux"

        cls._is_agent_server = agent_server

        if not cls.is_agent_server():
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
            cls._lib_type = ctypes.WinDLL
        else:
            cls._lib_type = ctypes.CDLL

        if cls._lib_type is None:
            raise

        if not cls.is_agent_server():
            cls.framework_libpath = path / framework_library[platform_type]
            cls.agent_client_libpath = path / agent_client_library[platform_type]
        else:
            cls.agent_server_libpath = path / agent_server_library[platform_type]

        cls.toolkit_libpath = path / toolkit_library[platform_type]

    @classmethod
    def framework(cls) -> Union["ctypes.CDLL", "ctypes.WinDLL"]:
        """获取 MaaFramework 库 / Get MaaFramework library

        Returns:
            (ctypes.CDLL | ctypes.WinDLL): MaaFramework 动态库对象 / MaaFramework dynamic library object
        """
        if cls._lib_type is None:
            raise RuntimeError("Library._lib_type is None!")

        if not cls.is_agent_server():
            if not cls._framework:
                cls._framework = cls._lib_type(str(cls.framework_libpath))

            if cls._framework is None:
                raise RuntimeError("Library._framework is None!")

            return cls._framework
        else:
            return cls.agent_server()

    @classmethod
    def toolkit(cls) -> Union["ctypes.CDLL", "ctypes.WinDLL"]:
        """获取 MaaToolkit 库 / Get MaaToolkit library

        Returns:
            (ctypes.CDLL | ctypes.WinDLL): MaaFramework 动态库对象 / MaaFramework dynamic library object
        """
        if cls._lib_type is None:
            raise RuntimeError("Library._lib_type is None!")

        if not cls._toolkit:
            cls._toolkit = cls._lib_type(str(cls.toolkit_libpath))

        return cls._toolkit

    @classmethod
    def agent_client(cls) -> Union["ctypes.CDLL", "ctypes.WinDLL"]:
        """获取 MaaAgentClient 库 / Get MaaAgentClient library

        Returns:
            (ctypes.CDLL | ctypes.WinDLL): MaaFramework 动态库对象 / MaaFramework dynamic library object

        Raises:
            ValueError: 如果在 AgentServer 模式下调用
        """
        if cls._lib_type is None:
            raise RuntimeError("Library._lib_type is None!")

        if cls.is_agent_server():
            raise ValueError("Agent server is not available in the current context.")

        if not cls._agent_client:
            cls._agent_client = cls._lib_type(str(cls.agent_client_libpath))

        return cls._agent_client

    @classmethod
    def agent_server(cls) -> Union["ctypes.CDLL", "ctypes.WinDLL"]:
        """获取 MaaAgentServer 库 / Get MaaAgentServer library

        Returns:
            (ctypes.CDLL | ctypes.WinDLL): MaaAgentServer 动态库对象 / MaaAgentServer dynamic library object

        Raises:
            ValueError: 如果不在 AgentServer 模式下调用
        """
        if cls._lib_type is None:
            raise RuntimeError("Library._lib_type is None!")

        if not cls.is_agent_server():
            raise ValueError("Agent client is not available in the current context.")

        if not cls._agent_server:
            cls._agent_server = cls._lib_type(str(cls.agent_server_libpath))

        return cls._agent_server

    @classmethod
    def is_agent_server(cls) -> bool:
        """判断是否为 AgentServer 模式 / Check if in AgentServer mode

        Returns:
            bool: 是否为 AgentServer 模式 / Whether in AgentServer mode
        """
        return cls._is_agent_server

    @classmethod
    def version(cls) -> str:
        """获取 MaaFramework 版本 / Get MaaFramework version

        Returns:
            str: 版本字符串 / Version string
        """
        cls._set_api_properties()

        return cls.framework().MaaVersion().decode()

    _api_properties_initialized: bool = False

    @classmethod
    def _set_api_properties(cls):
        if cls._api_properties_initialized:
            return

        cls._api_properties_initialized = True

        cls.framework().MaaVersion.restype = ctypes.c_char_p
        cls.framework().MaaVersion.argtypes = []
