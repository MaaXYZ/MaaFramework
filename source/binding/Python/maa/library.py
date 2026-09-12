import ctypes
import pathlib
import platform
import threading
from typing import Optional

from .define import *


class Library:
    """库加载管理器 / Library loading manager

    管理 MaaFramework 各动态库的加载和访问。
    Manages loading and access to MaaFramework dynamic libraries.
    """

    # 保护各绑定类 ctypes argtypes/restype 的一次性初始化（见 #629）。
    #
    # 这些赋值必须在 _api_properties_initialized 置位之前全部完成：初始化的
    # 第一条语句就会触发下方动态库的懒加载，而 WinDLL()/CDLL() 会执行磁盘 I/O
    # 并释放 GIL。若先置位，另一线程会在此刻跳过初始化，转而调用 argtypes 仍为
    # None 的函数——ctypes 按 32 位 c_int 处理，64 位句柄或抛 OverflowError，
    # 或被静默截断后由 C 侧解引用。
    #
    # 用 RLock 而非 Lock：初始化路径若将来出现嵌套，死锁远比多余的可重入性难查。
    _api_lock: threading.RLock = threading.RLock()

    _is_agent_server: bool = False

    _framework: Optional[ctypes.CDLL] = None
    _toolkit: Optional[ctypes.CDLL] = None
    _agent_client: Optional[ctypes.CDLL] = None
    _agent_server: Optional[ctypes.CDLL] = None
    _lib_type: Optional[type[ctypes.CDLL]] = None

    framework_libpath: Optional[pathlib.Path] = None
    toolkit_libpath: Optional[pathlib.Path] = None
    agent_client_libpath: Optional[pathlib.Path] = None
    agent_server_libpath: Optional[pathlib.Path] = None

    @classmethod
    def open(cls, path: pathlib.Path, agent_server: bool = False) -> None:
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

        platform_type = platform.system().lower()
        cls._lib_type = ctypes.WinDLL if platform_type == WINDOWS else ctypes.CDLL

        if not cls.is_agent_server():
            cls.framework_libpath = (
                path
                / {
                    WINDOWS: "MaaFramework.dll",
                    MACOS: "libMaaFramework.dylib",
                    LINUX: "libMaaFramework.so",
                }[platform_type]
            )
            cls.agent_client_libpath = (
                path
                / {
                    WINDOWS: "MaaAgentClient.dll",
                    MACOS: "libMaaAgentClient.dylib",
                    LINUX: "libMaaAgentClient.so",
                }[platform_type]
            )
            cls.toolkit_libpath = (
                path
                / {
                    WINDOWS: "MaaToolkit.dll",
                    MACOS: "libMaaToolkit.dylib",
                    LINUX: "libMaaToolkit.so",
                }[platform_type]
            )
        else:
            cls.agent_server_libpath = (
                path
                / {
                    WINDOWS: "MaaAgentServer.dll",
                    MACOS: "libMaaAgentServer.dylib",
                    LINUX: "libMaaAgentServer.so",
                }[platform_type]
            )

    @classmethod
    def framework(cls) -> ctypes.CDLL:
        """获取 MaaFramework 库 / Get MaaFramework library

        Returns:
            (ctypes.CDLL | ctypes.WinDLL): MaaFramework 动态库对象 / MaaFramework dynamic library object
        """
        if cls._lib_type is None:
            raise RuntimeError("Library._lib_type is None!")

        if not cls.is_agent_server():
            if not cls._framework:
                cls._framework = cls._lib_type(str(cls.framework_libpath))

            return cls._framework
        else:
            return cls.agent_server()

    @classmethod
    def toolkit(cls) -> ctypes.CDLL:
        """获取 MaaToolkit 库 / Get MaaToolkit library

        Returns:
            (ctypes.CDLL | ctypes.WinDLL): MaaToolkit 动态库对象 / MaaToolkit dynamic library object

        Raises:
            ValueError: 如果在 AgentServer 模式下调用
        """
        if cls._lib_type is None:
            raise RuntimeError("Library._lib_type is None!")

        if cls.is_agent_server():
            raise ValueError("Toolkit is not available in AgentServer context.")

        if not cls._toolkit:
            cls._toolkit = cls._lib_type(str(cls.toolkit_libpath))

        return cls._toolkit

    @classmethod
    def agent_client(cls) -> ctypes.CDLL:
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
    def agent_server(cls) -> ctypes.CDLL:
        """获取 MaaAgentServer 库 / Get MaaAgentServer library

        Returns:
            (ctypes.CDLL | ctypes.WinDLL): MaaAgentServer 动态库对象
            MaaAgentServer dynamic library object

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
    def _set_api_properties(cls) -> None:
        if cls._api_properties_initialized:
            return

        with cls._api_lock:
            if cls._api_properties_initialized:
                return

            cls.framework().MaaVersion.restype = ctypes.c_char_p
            cls.framework().MaaVersion.argtypes = []

            cls._api_properties_initialized = True
