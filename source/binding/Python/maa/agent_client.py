import ctypes

from .define import *
from .library import Library
from .resource import Resource
from .buffer import StringListBuffer


class AgentClient:
    _handle: MaaAgentClientHandle

    def __init__(self):
        self._set_api_properties()

        self._handle = Library.agent_client.MaaAgentClientCreate()
        if not self._handle:
            raise RuntimeError("Failed to create agent client.")

    def __del__(self):
        if self._handle:
            Library.agent_client.MaaAgentClientDestroy(self._handle)

    def bind(self, resource: Resource) -> bool:
        # avoid gc
        self._resource = resource

        return bool(Library.agent_client.MaaAgentClientBind(self._handle, resource._handle))

    def start_child(self, child_exec: str, child_args: list[str]) -> bool:
        args_buffer = StringListBuffer()
        args_buffer.set(child_args)

        return bool(
            Library.agent_client.MaaAgentClientStartChild(
                self._handle, child_exec.encode(), args_buffer._handle
            )
        )

    _api_properties_initialized: bool = False

    @staticmethod
    def _set_api_properties():
        if AgentClient._api_properties_initialized:
            return

        if Library.is_agent_server:
            raise RuntimeError("AgentClient is not available in AgentServer.")

        AgentClient._api_properties_initialized = True

        Library.agent_client.MaaAgentClientCreate.argtypes = []
        Library.agent_client.MaaAgentClientCreate.restype = MaaAgentClientHandle

        Library.agent_client.MaaAgentClientDestroy.argtypes = [MaaAgentClientHandle]
        Library.agent_client.MaaAgentClientDestroy.restype = None

        Library.agent_client.MaaAgentClientBindResource.argtypes = [MaaAgentClientHandle, MaaResourceHandle]
        Library.agent_client.MaaAgentClientBindResource.restype = MaaBool

        Library.agent_client.MaaAgentClientStartChild.argtypes = [
            MaaAgentClientHandle,
            ctypes.c_char_p,
            ctypes.c_void_p,
        ]
        Library.agent_client.MaaAgentClientStartChild.restype = MaaBool
