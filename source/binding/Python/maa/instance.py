import ctypes
import json
import asyncio
from typing import Union, Optional, Any

from .define import MaaApiCallback, MaaBool, MaaId, MaaStatus
from .common import Status
from .library import Library
from .callback_agent import CallbackAgent, Callback
from .controller import Controller
from .resource import Resource
from .custom_recognizer import CustomRecognizer
from .custom_action import CustomAction


class Instance:
    _callback_agent: CallbackAgent
    _handle: ctypes.c_void_p

    def __init__(self, callback: Optional[Callback] = None, callback_arg: Any = None):
        """
        Create a new MAA task manager.

        :param callback: The callback function.
        :param callback_arg: The callback argument.
        """

        if not Library.initialized:
            raise RuntimeError(
                "Library not initialized, please call `library.open()` first."
            )

        self._set_api_properties()

        self._callback_agent = CallbackAgent(callback, callback_arg)
        self._handle = Library.framework.MaaCreate(
            self._callback_agent.c_callback(), self._callback_agent.c_callback_arg()
        )

        if not self._handle:
            raise RuntimeError("Failed to create resource.")

    def __del__(self):
        """
        Destroy the MAA task manager.
        """
        if self._handle:
            Library.framework.MaaDestroy(self._handle)

    def bind(self, resource: Resource, controller: Controller) -> bool:
        """
        Bind the resource and controller to the instance.

        :param resource: The resource to bind.
        :param controller: The controller to bind.
        :return: True if the resource and controller were successfully bound, False otherwise.
        """

        return Library.framework.MaaBindResource(
            self._handle, resource._handle
        ) and Library.framework.MaaBindController(self._handle, controller._handle)

    def inited(self) -> bool:
        """
        Check if the instance is inited.

        :return: True if the instance is inited, False otherwise.
        """

        return Library.framework.MaaInited(self._handle)

    async def run_task(self, task_type: str, param: Any = {}) -> bool:
        """
        Run a task.

        :param task_type: The name of the task.
        :param param: The param of the task.
        :return: True if the task was successfully run, False otherwise.
        """

        tid = self.post_task(task_type, param)
        while not self.status(tid).done():
            await asyncio.sleep(0)

        return self.status(tid).success()

    def post_task(self, task_type: str, param: Any = {}) -> int:
        """
        Post a task to the instance.

        :param task_type: The name of the task.
        :param param: The param of the task.
        :return: The id of the posted task.
        """

        return Library.framework.MaaPostTask(
            self._handle, task_type.encode("utf-8"), json.dumps(param).encode("utf-8")
        )

    def set_task_param(self, id: int, param: Any) -> bool:
        """
        Set the param of the given task.

        :param id: The id of the task.
        :param param: The param of the task.
        :return: True if the param was successfully set, False otherwise.
        """

        return Library.framework.MaaSetTaskParam(
            self._handle, id, json.dumps(param).encode("utf-8")
        )

    def status(self, id: int) -> Status:
        """
        Get the status of the given id.

        :param id: The id.
        :return: The status of the given id.
        """

        return Status(Library.framework.MaaTaskStatus(self._handle, id))

    def wait(self, id: int) -> Status:
        """
        Wait for the given id to complete.

        :param id: The id.
        :return: The status of the given id.
        """

        return Status(Library.framework.MaaWaitTask(self._handle, id))

    async def wait_all(self):
        """
        Wait for all tasks to complete.
        """

        while not self.all_finished():
            await asyncio.sleep(0)

    def all_finished(self) -> bool:
        """
        Check if all tasks are finished.

        :return: True if all tasks are finished, False otherwise.
        """

        return bool(Library.framework.MaaTaskAllFinished(self._handle))

    def stop(self) -> bool:
        """
        Stop all tasks.

        :return: True if all tasks were successfully stopped, False otherwise.
        """

        return bool(Library.framework.MaaStop(self._handle))

    def register_recognizer(self, name: str, recognizer: CustomRecognizer) -> bool:
        """
        Register a custom recognizer.

        :param name: The name of the custom recognizer.
        :param recognizer: The custom recognizer.
        :return: True if the custom recognizer was successfully registered, False otherwise.
        """

        return bool(
            Library.framework.MaaRegisterCustomRecognizer(
                self._handle,
                name.encode("utf-8"),
                recognizer.c_handle(),
                recognizer.c_arg(),
            )
        )

    def register_action(self, name: str, action: CustomAction) -> bool:
        """
        Register a custom action.

        :param name: The name of the custom action.
        :param action: The custom action.
        :return: True if the custom action was successfully registered, False otherwise.
        """

        return bool(
            Library.framework.MaaRegisterCustomAction(
                self._handle,
                name.encode("utf-8"),
                action.c_handle(),
                action.c_arg(),
            )
        )

    def c_handle(self) -> ctypes.c_void_p:
        """
        Get the C handle of the instance.

        :return: The C handle of the instance.
        """

        return self._handle

    _api_properties_initialized: bool = False

    @staticmethod
    def _set_api_properties():
        """
        Set the API properties.
        """
        if Instance._api_properties_initialized:
            return
        Instance._api_properties_initialized = True

        Library.framework.MaaCreate.restype = ctypes.c_void_p
        Library.framework.MaaCreate.argtypes = [MaaApiCallback, ctypes.c_void_p]

        Library.framework.MaaDestroy.restype = None
        Library.framework.MaaDestroy.argtypes = [ctypes.c_void_p]

        Library.framework.MaaBindResource.restype = MaaBool
        Library.framework.MaaBindResource.argtypes = [ctypes.c_void_p, ctypes.c_void_p]

        Library.framework.MaaBindController.restype = MaaBool
        Library.framework.MaaBindController.argtypes = [
            ctypes.c_void_p,
            ctypes.c_void_p,
        ]

        Library.framework.MaaInited.restype = MaaBool
        Library.framework.MaaInited.argtypes = [ctypes.c_void_p]

        Library.framework.MaaPostTask.restype = MaaId
        Library.framework.MaaPostTask.argtypes = [
            ctypes.c_void_p,
            ctypes.c_char_p,
            ctypes.c_char_p,
        ]

        Library.framework.MaaSetTaskParam.restype = MaaBool
        Library.framework.MaaSetTaskParam.argtypes = [
            ctypes.c_void_p,
            MaaId,
            ctypes.c_char_p,
        ]

        Library.framework.MaaTaskStatus.restype = MaaStatus
        Library.framework.MaaTaskStatus.argtypes = [ctypes.c_void_p, MaaId]

        Library.framework.MaaWaitTask.restype = MaaStatus
        Library.framework.MaaWaitTask.argtypes = [ctypes.c_void_p, MaaId]

        Library.framework.MaaTaskAllFinished.restype = MaaBool
        Library.framework.MaaTaskAllFinished.argtypes = [ctypes.c_void_p]

        Library.framework.MaaStop.restype = MaaBool
        Library.framework.MaaStop.argtypes = [ctypes.c_void_p]

        Library.framework.MaaRegisterCustomRecognizer.restype = MaaBool
        Library.framework.MaaRegisterCustomRecognizer.argtypes = [
            ctypes.c_void_p,
            ctypes.c_char_p,
            ctypes.c_void_p,
            ctypes.c_void_p,
        ]

        Library.framework.MaaRegisterCustomAction.restype = MaaBool
        Library.framework.MaaRegisterCustomAction.argtypes = [
            ctypes.c_void_p,
            ctypes.c_char_p,
            ctypes.c_void_p,
            ctypes.c_void_p,
        ]
