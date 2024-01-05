import ctypes
import pathlib
import asyncio
from typing import Union, Optional, Any

from .define import *
from .future import Future
from .library import Library
from .callback_agent import CallbackAgent, Callback


class Resource:
    _handle: MaaResourceHandle
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

    async def load(self, path: Union[pathlib.Path, str]) -> bool:
        """
        Async load the given path to the resource.

        :param path: The path to load.
        :return: True if the resource was successfully loaded, False otherwise.
        """

        await self.post_path(path).wait()

    def post_path(self, path: Union[pathlib.Path, str]) -> Future:
        """
        Post a path to the resource. (load in background)

        :param path: The path to post.
        :return: The id of the posted path.
        """

        maaid = Library.framework.MaaResourcePostPath(
            self._handle, str(path).encode("utf-8")
        )
        return Future(maaid, self._status)

    def loaded(self) -> bool:
        """
        Check if the resource is loaded.

        :return: True if the resource is loaded, False otherwise.
        """

        return bool(Library.framework.MaaResourceLoaded(self._handle))

    def _status(self, id: int) -> ctypes.c_int32:
        return Library.framework.MaaResourceStatus(self._handle, id)

    _api_properties_initialized: bool = False

    @staticmethod
    def _set_api_properties():
        """
        Set the API properties for this resource.
        """
        if Resource._api_properties_initialized:
            return
        Resource._api_properties_initialized = True

        Library.framework.MaaResourceCreate.restype = MaaResourceHandle
        Library.framework.MaaResourceCreate.argtypes = [
            MaaResourceCallback,
            MaaCallbackTransparentArg,
        ]

        Library.framework.MaaResourceDestroy.restype = None
        Library.framework.MaaResourceDestroy.argtypes = [MaaResourceHandle]

        Library.framework.MaaResourcePostPath.restype = MaaResId
        Library.framework.MaaResourcePostPath.argtypes = [
            MaaResourceHandle,
            MaaStringView,
        ]

        Library.framework.MaaResourceStatus.restype = MaaStatus
        Library.framework.MaaResourceStatus.argtypes = [
            MaaResourceHandle,
            MaaResId,
        ]

        Library.framework.MaaResourceLoaded.restype = MaaBool
        Library.framework.MaaResourceLoaded.argtypes = [MaaResourceHandle]
