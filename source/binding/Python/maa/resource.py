import ctypes
import pathlib
from typing import Any, Optional, Union

from .callback_agent import Callback, CallbackAgent
from .define import *
from .job import Job
from .library import Library
from .buffer import StringBuffer


class Resource:
    _handle: MaaResourceHandle
    _callback_agent: CallbackAgent
    _own: bool = False

    ### public ###

    def __init__(
        self,
        callback: Optional[Callback] = None,
        callback_arg: Any = None,
        handle: Optional[MaaResourceHandle] = None,
    ):

        if not Library.initialized:
            raise RuntimeError(
                "Library not initialized, please call `library.open()` first."
            )

        self._set_api_properties()

        if handle:
            self._handle = handle
            self._own = False
        else:
            self._callback_agent = CallbackAgent(callback, callback_arg)
            self._handle = Library.framework.MaaResourceCreate(
                self._callback_agent.c_callback, self._callback_agent.c_callback_arg
            )
            self._own = True

        if not self._handle:
            raise RuntimeError("Failed to create resource.")

        self._custom_action_holder = {}
        self._custom_recognizer_holder = {}

    def __del__(self):
        if self._handle and self._own:
            Library.framework.MaaResourceDestroy(self._handle)

    def post_path(self, path: Union[pathlib.Path, str]) -> Job:
        resid = Library.framework.MaaResourcePostPath(self._handle, str(path).encode())
        return Job(resid, self._status, self._wait)

    @property
    def loaded(self) -> bool:
        return bool(Library.framework.MaaResourceLoaded(self._handle))

    def clear(self) -> bool:
        return bool(Library.framework.MaaResourceClear(self._handle))

    def register_custom_recognizer(
        self, name: str, recognizer: "CustomRecognizer"  # type: ignore
    ) -> bool:

        # avoid gc
        self._custom_recognizer_holder[name] = recognizer

        return bool(
            Library.framework.MaaResourceRegisterCustomRecognizer(
                self._handle,
                name.encode(),
                recognizer.c_handle,
                recognizer.c_arg,
            )
        )

    def unregister_custom_recognizer(self, name: str) -> bool:
        self._custom_recognizer_holder.pop(name, None)

        return bool(
            Library.framework.MaaResourceUnregisterCustomRecognizer(
                self._handle,
                name.encode(),
            )
        )

    def clear_custom_recognizer(self) -> bool:
        self._custom_recognizer_holder.clear()

        return bool(
            Library.framework.MaaResourceClearCustomRecognizer(
                self._handle,
            )
        )

    def register_custom_action(self, name: str, action: "CustomAction") -> bool:  # type: ignore
        # avoid gc
        self._custom_action_holder[name] = action

        return bool(
            Library.framework.MaaResourceRegisterCustomAction(
                self._handle,
                name.encode(),
                action.c_handle,
                action.c_arg,
            )
        )

    def unregister_custom_action(self, name: str) -> bool:
        self._custom_action_holder.pop(name, None)

        return bool(
            Library.framework.MaaResourceUnregisterCustomAction(
                self._handle,
                name.encode(),
            )
        )

    def clear_custom_action(self) -> bool:
        self._custom_action_holder.clear()

        return bool(
            Library.framework.MaaResourceClearCustomAction(
                self._handle,
            )
        )

    @property
    def hash(self) -> str:
        buffer = StringBuffer()
        if not Library.framework.MaaResourceGetHash(self._handle, buffer._handle):
            raise RuntimeError("Failed to get hash.")
        return buffer.get()

    ### private ###

    def _status(self, id: int) -> ctypes.c_int32:
        return Library.framework.MaaResourceStatus(self._handle, id)

    def _wait(self, id: int) -> ctypes.c_int32:
        return Library.framework.MaaResourceWait(self._handle, id)

    _api_properties_initialized: bool = False

    @staticmethod
    def _set_api_properties():
        if Resource._api_properties_initialized:
            return
        Resource._api_properties_initialized = True

        Library.framework.MaaResourceCreate.restype = MaaResourceHandle
        Library.framework.MaaResourceCreate.argtypes = [
            MaaNotificationCallback,
            ctypes.c_void_p,
        ]

        Library.framework.MaaResourceDestroy.restype = None
        Library.framework.MaaResourceDestroy.argtypes = [MaaResourceHandle]

        Library.framework.MaaResourcePostPath.restype = MaaResId
        Library.framework.MaaResourcePostPath.argtypes = [
            MaaResourceHandle,
            ctypes.c_char_p,
        ]

        Library.framework.MaaResourceStatus.restype = MaaStatus
        Library.framework.MaaResourceStatus.argtypes = [
            MaaResourceHandle,
            MaaResId,
        ]

        Library.framework.MaaResourceWait.restype = MaaStatus
        Library.framework.MaaResourceWait.argtypes = [
            MaaResourceHandle,
            MaaResId,
        ]

        Library.framework.MaaResourceLoaded.restype = MaaBool
        Library.framework.MaaResourceLoaded.argtypes = [MaaResourceHandle]

        Library.framework.MaaResourceClear.restype = MaaBool
        Library.framework.MaaResourceClear.argtypes = [MaaResourceHandle]

        Library.framework.MaaResourceGetHash.restype = MaaBool
        Library.framework.MaaResourceGetHash.argtypes = [
            MaaResourceHandle,
            MaaStringBufferHandle,
        ]

        Library.framework.MaaResourceRegisterCustomRecognizer.restype = MaaBool
        Library.framework.MaaResourceRegisterCustomRecognizer.argtypes = [
            MaaResourceHandle,
            ctypes.c_char_p,
            MaaCustomRecognizerCallback,
            ctypes.c_void_p,
        ]

        Library.framework.MaaResourceUnregisterCustomRecognizer.restype = MaaBool
        Library.framework.MaaResourceUnregisterCustomRecognizer.argtypes = [
            MaaResourceHandle,
            ctypes.c_char_p,
        ]

        Library.framework.MaaResourceClearCustomRecognizer.restype = MaaBool
        Library.framework.MaaResourceClearCustomRecognizer.argtypes = [
            MaaResourceHandle,
        ]

        Library.framework.MaaResourceRegisterCustomAction.restype = MaaBool
        Library.framework.MaaResourceRegisterCustomAction.argtypes = [
            MaaResourceHandle,
            ctypes.c_char_p,
            MaaCustomActionCallback,
            ctypes.c_void_p,
        ]

        Library.framework.MaaResourceUnregisterCustomAction.restype = MaaBool
        Library.framework.MaaResourceUnregisterCustomAction.argtypes = [
            MaaResourceHandle,
            ctypes.c_char_p,
        ]

        Library.framework.MaaResourceClearCustomAction.restype = MaaBool
        Library.framework.MaaResourceClearCustomAction.argtypes = [
            MaaResourceHandle,
        ]
