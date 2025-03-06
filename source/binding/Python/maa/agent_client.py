import ctypes

from .define import *
from .library import Library
from .resource import Resource
from .buffer import StringBuffer


class AgentClient:
    _handle: MaaAgentClientHandle

    def __init__(self):
        self._set_api_properties()

        self._handle = Library.agent_client().MaaAgentClientCreate()
        if not self._handle:
            raise RuntimeError("Failed to create agent client.")

    def __del__(self):
        if self._handle:
            Library.agent_client().MaaAgentClientDestroy(self._handle)

    def bind(self, resource: Resource) -> bool:
        # avoid gc
        self._resource = resource

        return bool(
            Library.agent_client().MaaAgentClientBindResource(
                self._handle, resource._handle
            )
        )

    def create_socket(self, identifier: str = "") -> Optional[str]:
        id_buffer = StringBuffer()
        id_buffer.set(identifier)

        ret = bool(
            Library.agent_client().MaaAgentClientCreateSocket(
                self._handle, id_buffer._handle
            )
        )
        if not ret:
            return None

        return id_buffer.get()

    def connect(self) -> bool:
        return bool(Library.agent_client().MaaAgentClientConnect(self._handle))

    def disconnect(self) -> bool:
        return bool(Library.agent_client().MaaAgentClientDisconnect(self._handle))

    _api_properties_initialized: bool = False

    @staticmethod
    def _set_api_properties():
        if AgentClient._api_properties_initialized:
            return

        if Library.is_agent_server():
            raise RuntimeError("AgentClient is not available in AgentServer.")

        AgentClient._api_properties_initialized = True

        Library.agent_client().MaaAgentClientCreate.restype = MaaAgentClientHandle
        Library.agent_client().MaaAgentClientCreate.argtypes = []

        Library.agent_client().MaaAgentClientDestroy.restype = None
        Library.agent_client().MaaAgentClientDestroy.argtypes = [MaaAgentClientHandle]

        Library.agent_client().MaaAgentClientBindResource.restype = MaaBool
        Library.agent_client().MaaAgentClientBindResource.argtypes = [
            MaaAgentClientHandle,
            MaaResourceHandle,
        ]

        Library.agent_client().MaaAgentClientCreateSocket.restype = MaaBool
        Library.agent_client().MaaAgentClientCreateSocket.argtypes = [
            MaaAgentClientHandle,
            MaaStringBufferHandle,
        ]

        Library.agent_client().MaaAgentClientConnect.restype = MaaBool
        Library.agent_client().MaaAgentClientConnect.argtypes = [
            MaaAgentClientHandle,
        ]

        Library.agent_client().MaaAgentClientDisconnect.restype = MaaBool
        Library.agent_client().MaaAgentClientDisconnect.argtypes = [
            MaaAgentClientHandle,
        ]
