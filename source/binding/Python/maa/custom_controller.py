import ctypes
from abc import ABC, abstractmethod
from typing import Optional

from .define import *
from .buffer import RectBuffer, StringBuffer
from .context import SyncContext


class CustomControllerAgent(ABC):
    _api: MaaCustomControllerAPI

    def __init__(self):
        self._api = MaaCustomControllerAPI(
            self._c_connect_agent,
            self._c_request_uuid_agent,
            # TODO
        )

    @property
    def c_handle(self) -> MaaCustomControllerHandle:
        return MaaCustomControllerHandle(self._api)

    @property
    def c_arg(self) -> MaaTransparentArg:
        return MaaTransparentArg.from_buffer(ctypes.py_object(self))

    @abstractmethod
    def connect(self) -> bool:
        raise NotImplementedError

    @abstractmethod
    def request_uuid(self) -> Optional[str]:
        raise NotImplementedError

    @MaaCustomControllerAPI.ConnectFunc
    def _c_connect_agent(
        c_handle_arg: MaaTransparentArg,
    ) -> MaaBool:
        if not c_handle_arg:
            return

        self: CustomControllerAgent = ctypes.cast(
            c_handle_arg,
            ctypes.py_object,
        ).value

        return self.connect()

    @MaaCustomControllerAPI.RequestUuidFunc
    def _c_request_uuid_agent(
        c_handle_arg: MaaTransparentArg,
        buffer: MaaStringBufferHandle,
    ):
        if not c_handle_arg:
            return

        self: CustomControllerAgent = ctypes.cast(
            c_handle_arg,
            ctypes.py_object,
        ).value

        uuid_buffer = StringBuffer(buffer)
        uuid = self.request_uuid()

        if not uuid:
            return False

        uuid_buffer.write(uuid)
        return True
