import ctypes
import pathlib
from typing import Union, Optional, Any

from .define import MaaApiCallback, MaaBool, MaaId, MaaStatus
from .common import Status
from .library import Library
from .callback_agent import CallbackAgent, Callback


class Resource:
    _handle: ctypes.c_void_p
    _callback_agent: CallbackAgent

    def __init__(self, callback: Optional[Callback] = None, callback_arg: Any = None):
        """
        Create a new resource object.

        :param callback: The callback function.
        :param callback_arg: The callback argument.
        """

        if not Library.initialized:
            raise RuntimeError(
                "Library not initialized, please call `library.open()` first."
            )

        self._set_api_properties()

        self._callback_agent = CallbackAgent(callback, callback_arg)
        self._handle = Library.framework.MaaResourceCreate(
            self._callback_agent.c_callback(), self._callback_agent.c_callback_arg()
        )

        if not self._handle:
            raise RuntimeError("Failed to create resource.")

    def __del__(self):
        """
        Destroy the resource object.
        """
        if self._handle:
            Library.framework.MaaResourceDestroy(self._handle)

    def load(self, path: Union[pathlib.Path, str]) -> bool:
        """
        Sync load the given path to the resource.

        :param path: The path to load.
        :return: True if the resource was successfully loaded, False otherwise.
        """

        rid = self.post_path(path)
        return self.wait(rid) == Status.success

    def post_path(self, path: Union[pathlib.Path, str]) -> int:
        """
        Async load the given path to the resource.

        :param path: The path to post.
        :return: The id of the posted path.
        """

        return Library.framework.MaaResourcePostPath(
            self._handle, str(path).encode("utf-8")
        )

    def status(self, id: int) -> Status:
        """
        Get the status of the given id.

        :param id: The id.
        :return: The status of the given id.
        """

        return Status(Library.framework.MaaResourceStatus(self._handle, id))

    def wait(self, id: int) -> Status:
        """
        Wait for the given id to complete.

        :param id: The id.
        :return: The status of the given id.
        """

        return Status(Library.framework.MaaResourceWait(self._handle, id))

    def loaded(self) -> bool:
        """
        Check if the resource is loaded.

        :return: True if the resource is loaded, False otherwise.
        """

        return bool(Library.framework.MaaResourceLoaded(self._handle))

    _api_properties_initialized: bool = False

    @staticmethod
    def _set_api_properties():
        """
        Set the API properties for this resource.
        """
        if Resource._api_properties_initialized:
            return
        Resource._api_properties_initialized = True

        Library.framework.MaaResourceCreate.restype = ctypes.c_void_p
        Library.framework.MaaResourceCreate.argtypes = [MaaApiCallback, ctypes.c_void_p]

        Library.framework.MaaResourceDestroy.restype = None
        Library.framework.MaaResourceDestroy.argtypes = [ctypes.c_void_p]

        Library.framework.MaaResourcePostPath.restype = MaaId
        Library.framework.MaaResourcePostPath.argtypes = [
            ctypes.c_void_p,
            ctypes.c_char_p,
        ]

        Library.framework.MaaResourceStatus.restype = MaaStatus
        Library.framework.MaaResourceStatus.argtypes = [ctypes.c_void_p, MaaId]

        Library.framework.MaaResourceWait.restype = MaaStatus
        Library.framework.MaaResourceWait.argtypes = [ctypes.c_void_p, MaaId]

        Library.framework.MaaResourceLoaded.restype = MaaBool
        Library.framework.MaaResourceLoaded.argtypes = [ctypes.c_void_p]
