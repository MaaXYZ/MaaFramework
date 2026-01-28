import ctypes
from typing import List

from .define import *
from .library import Library
from .resource import Resource
from .controller import Controller
from .tasker import Tasker
from .buffer import StringBuffer, StringListBuffer


class AgentClient:
    """Agent 客户端 / Agent client

    用于连接到 AgentServer，将自定义识别器和动作的执行委托给独立进程。
    这允许将 MaaFW 本体与 Custom 逻辑分离至独立进程运行。
    Used to connect to AgentServer, delegating custom recognition and action execution to a separate process.
    This allows separating MaaFW core from Custom logic into independent processes.
    """

    _handle: MaaAgentClientHandle

    def __init__(self, identifier: Optional[str] = None, *, _handle: MaaAgentClientHandle = None):
        """创建 Agent 客户端 / Create Agent client

        默认使用 IPC 模式。在不支持 AF_UNIX 的旧版 Windows（Build 17063 之前）上会自动回退到 TCP 模式。
        Uses IPC mode by default. On older Windows versions that don't support AF_UNIX (before Build 17063),
        it will automatically fall back to TCP mode.

        Args:
            identifier: 可选的连接标识符，用于匹配特定的 AgentServer / Optional connection identifier for matching specific AgentServer
            _handle: 内部使用，直接传入已创建的句柄 / Internal use, directly pass in an already created handle

        Raises:
            RuntimeError: 如果创建失败
        """
        self._set_api_properties()

        if _handle:
            self._handle = _handle
            return

        if identifier:
            id_buffer = StringBuffer()
            id_buffer.set(identifier)
            id_buffer_handle = id_buffer._handle
        else:
            id_buffer_handle = None

        self._handle = Library.agent_client().MaaAgentClientCreateV2(id_buffer_handle)
        if not self._handle:
            raise RuntimeError("Failed to create agent client.")

    @classmethod
    def create_tcp(cls, port: int = 0) -> "AgentClient":
        """创建使用 TCP 连接的 Agent 客户端 / Create Agent client with TCP connection

        客户端会监听 127.0.0.1 上的指定端口。如果传入 0 则自动选择可用端口。
        AgentServer 端使用 identifier 属性获取的端口号作为 identifier 即可通过 TCP 连接。
        The client listens on 127.0.0.1 at the specified port. If 0 is passed, an available port is automatically selected.
        AgentServer can use the port number from the identifier property to connect via TCP.

        Args:
            port: TCP 端口号 (0-65535)，0 表示自动选择 / TCP port number (0-65535), 0 means auto-select

        Returns:
            AgentClient: TCP 模式的客户端实例 / Client instance in TCP mode

        Raises:
            RuntimeError: 如果创建失败
            ValueError: 如果端口号无效
        """
        if not (0 <= port <= 65535):
            raise ValueError(
                f"Invalid port number: {port}. Must be between 0 and 65535."
            )

        cls._set_api_properties()

        handle = Library.agent_client().MaaAgentClientCreateTcp(ctypes.c_uint16(port))
        if not handle:
            raise RuntimeError("Failed to create TCP agent client.")

        return cls(_handle=handle)

    def __del__(self):
        if self._handle:
            Library.agent_client().MaaAgentClientDestroy(self._handle)

    @property
    def identifier(self) -> Optional[str]:
        """获取连接标识符 / Get connection identifier

        Returns:
            Optional[str]: 连接标识符，如果未设置则返回 None / Connection identifier, or None if not set
        """
        id_buffer = StringBuffer()
        if not Library.agent_client().MaaAgentClientIdentifier(
            self._handle, id_buffer._handle
        ):
            return None

        return id_buffer.get()

    def bind(self, resource: Resource) -> bool:
        """绑定资源 / Bind resource

        将 AgentServer 中注册的自定义识别器和动作绑定到资源上。
        Bind custom recognitions and actions registered in AgentServer to the resource.

        Args:
            resource: 资源对象 / Resource object

        Returns:
            bool: 是否成功 / Whether successful
        """
        # avoid gc
        self._resource = resource

        return bool(
            Library.agent_client().MaaAgentClientBindResource(
                self._handle, resource._handle
            )
        )

    def register_sink(
        self, resource: Resource, controller: Controller, tasker: Tasker
    ) -> bool:
        """注册事件监听器 / Register event sinks

        将资源、控制器、任务器的事件转发给 AgentServer。
        Forward resource, controller, and tasker events to AgentServer.

        Args:
            resource: 资源对象 / Resource object
            controller: 控制器对象 / Controller object
            tasker: 任务器对象 / Tasker object

        Returns:
            bool: 是否全部注册成功 / Whether all registrations succeeded
        """
        # avoid gc
        self._sinks = [resource, controller, tasker]

        return (
            bool(
                Library.agent_client().MaaAgentClientRegisterResourceSink(
                    self._handle, resource._handle
                )
            )
            and bool(
                Library.agent_client().MaaAgentClientRegisterControllerSink(
                    self._handle, controller._handle
                )
            )
            and bool(
                Library.agent_client().MaaAgentClientRegisterTaskerSink(
                    self._handle, tasker._handle
                )
            )
        )

    def connect(self) -> bool:
        """连接到 AgentServer / Connect to AgentServer

        Returns:
            bool: 是否成功 / Whether successful
        """
        return bool(Library.agent_client().MaaAgentClientConnect(self._handle))

    def disconnect(self) -> bool:
        """断开与 AgentServer 的连接 / Disconnect from AgentServer

        Returns:
            bool: 是否成功 / Whether successful
        """
        return bool(Library.agent_client().MaaAgentClientDisconnect(self._handle))

    @property
    def connected(self) -> bool:
        """判断是否已连接 / Check if connected

        Returns:
            bool: 是否已连接 / Whether connected
        """
        return bool(Library.agent_client().MaaAgentClientConnected(self._handle))

    @property
    def alive(self) -> bool:
        """判断连接是否存活 / Check if connection is alive

        Returns:
            bool: 连接是否存活 / Whether connection is alive
        """
        return bool(Library.agent_client().MaaAgentClientAlive(self._handle))

    def set_timeout(self, milliseconds: int) -> bool:
        """设置超时时间 / Set timeout

        Args:
            milliseconds: 超时时间（毫秒） / Timeout in milliseconds

        Returns:
            bool: 是否成功 / Whether successful
        """
        return bool(
            Library.agent_client().MaaAgentClientSetTimeout(
                self._handle, ctypes.c_int64(milliseconds)
            )
        )

    @property
    def custom_recognition_list(self) -> List[str]:
        """获取已注册的自定义识别器列表 / Get registered custom recognizer list

        Returns:
            list[str]: 自定义识别器名列表 / List of custom recognizer names

        Raises:
            RuntimeError: 如果获取失败
        """
        buffer = StringListBuffer()
        if not Library.agent_client().MaaAgentClientGetCustomRecognitionList(
            self._handle, buffer._handle
        ):
            raise RuntimeError("Failed to get custom recognition list.")
        return buffer.get()

    @property
    def custom_action_list(self) -> List[str]:
        """获取已注册的自定义操作列表 / Get registered custom action list

        Returns:
            list[str]: 自定义操作名列表 / List of custom action names

        Raises:
            RuntimeError: 如果获取失败
        """
        buffer = StringListBuffer()
        if not Library.agent_client().MaaAgentClientGetCustomActionList(
            self._handle, buffer._handle
        ):
            raise RuntimeError("Failed to get custom action list.")
        return buffer.get()

    _api_properties_initialized: bool = False

    @staticmethod
    def _set_api_properties():
        if AgentClient._api_properties_initialized:
            return

        if Library.is_agent_server():
            raise RuntimeError("AgentClient is not available in AgentServer.")

        AgentClient._api_properties_initialized = True

        Library.agent_client().MaaAgentClientCreateV2.restype = MaaAgentClientHandle
        Library.agent_client().MaaAgentClientCreateV2.argtypes = [
            MaaStringBufferHandle,
        ]

        Library.agent_client().MaaAgentClientCreateTcp.restype = MaaAgentClientHandle
        Library.agent_client().MaaAgentClientCreateTcp.argtypes = [
            ctypes.c_uint16,
        ]

        Library.agent_client().MaaAgentClientIdentifier.restype = MaaBool
        Library.agent_client().MaaAgentClientIdentifier.argtypes = [
            MaaAgentClientHandle,
            MaaStringBufferHandle,
        ]

        Library.agent_client().MaaAgentClientDestroy.restype = None
        Library.agent_client().MaaAgentClientDestroy.argtypes = [MaaAgentClientHandle]

        Library.agent_client().MaaAgentClientBindResource.restype = MaaBool
        Library.agent_client().MaaAgentClientBindResource.argtypes = [
            MaaAgentClientHandle,
            MaaResourceHandle,
        ]

        Library.agent_client().MaaAgentClientRegisterResourceSink.restype = MaaBool
        Library.agent_client().MaaAgentClientRegisterResourceSink.argtypes = [
            MaaAgentClientHandle,
            MaaResourceHandle,
        ]

        Library.agent_client().MaaAgentClientRegisterControllerSink.restype = MaaBool
        Library.agent_client().MaaAgentClientRegisterControllerSink.argtypes = [
            MaaAgentClientHandle,
            MaaControllerHandle,
        ]

        Library.agent_client().MaaAgentClientRegisterTaskerSink.restype = MaaBool
        Library.agent_client().MaaAgentClientRegisterTaskerSink.argtypes = [
            MaaAgentClientHandle,
            MaaTaskerHandle,
        ]

        Library.agent_client().MaaAgentClientConnect.restype = MaaBool
        Library.agent_client().MaaAgentClientConnect.argtypes = [
            MaaAgentClientHandle,
        ]

        Library.agent_client().MaaAgentClientDisconnect.restype = MaaBool
        Library.agent_client().MaaAgentClientDisconnect.argtypes = [
            MaaAgentClientHandle,
        ]

        Library.agent_client().MaaAgentClientConnected.restype = MaaBool
        Library.agent_client().MaaAgentClientConnected.argtypes = [
            MaaAgentClientHandle,
        ]

        Library.agent_client().MaaAgentClientAlive.restype = MaaBool
        Library.agent_client().MaaAgentClientAlive.argtypes = [
            MaaAgentClientHandle,
        ]

        Library.agent_client().MaaAgentClientSetTimeout.restype = MaaBool
        Library.agent_client().MaaAgentClientSetTimeout.argtypes = [
            MaaAgentClientHandle,
            ctypes.c_int64,
        ]

        Library.agent_client().MaaAgentClientGetCustomRecognitionList.restype = MaaBool
        Library.agent_client().MaaAgentClientGetCustomRecognitionList.argtypes = [
            MaaAgentClientHandle,
            MaaStringListBufferHandle,
        ]

        Library.agent_client().MaaAgentClientGetCustomActionList.restype = MaaBool
        Library.agent_client().MaaAgentClientGetCustomActionList.argtypes = [
            MaaAgentClientHandle,
            MaaStringListBufferHandle,
        ]
