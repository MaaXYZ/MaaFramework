import asyncio
import ctypes
import json
from pathlib import Path
from typing import Any, Dict, Optional, Union

from .buffer import ImageListBuffer, RectBuffer, StringBuffer
from .callback_agent import Callback, CallbackAgent
from .controller import Controller
from .custom_action import CustomAction
from .custom_recognizer import CustomRecognizer
from .define import *
from .future import Future, MaaStatusEnum
from .library import Library
from .resource import Resource


@dataclass
class RecognitionDetail:
    reco_id: int
    hit_box: Optional[Rect]
    detail: Dict
    draws: List[numpy.ndarray]


class RunningDetail:
    run_id: int
    recognition: RecognitionDetail
    successful: bool


class Instance:
    _callback_agent: CallbackAgent
    _handle: MaaInstanceHandle

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
            self._callback_agent.c_callback, self._callback_agent.c_callback_arg
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

    @property
    def inited(self) -> bool:
        """
        Check if the instance is inited.

        :return: True if the instance is inited, False otherwise.
        """

        return Library.framework.MaaInited(self._handle)

    async def run_task(self, task_type: str, param: Dict = {}) -> bool:
        """
        Async run a task.

        :param task_type: The name of the task.
        :param param: The param of the task.
        :return: True if the task was successfully run, False otherwise.
        """

        return await self.post_task(task_type, param).wait()

    def post_task(self, task_type: str, param: Any = {}) -> Future:
        """
        Post a task to the instance. (run in background)

        :param task_type: The name of the task.
        :param param: The param of the task.
        :return: The id of the posted task.
        """

        maaid = Library.framework.MaaPostTask(
            self._handle, task_type.encode("utf-8"), json.dumps(param).encode("utf-8")
        )
        return TaskFuture(maaid, self._status, self._set_task_param)

    async def wait_all(self):
        """
        Wait for all tasks to complete.
        """

        while self.running():
            await asyncio.sleep(0)

    def running(self) -> bool:
        """
        Is running

        :return: True if running, False otherwise.
        """

        return bool(Library.framework.MaaRunning(self._handle))

    async def stop(self) -> bool:
        """
        Async stop all tasks.

        :return: True if all tasks were successfully stopped, False otherwise.
        """

        return await self.post_stop().wait()

    def post_stop(self) -> Future:
        """
        Stop all tasks.

        :return: True if all tasks were successfully stopped, False otherwise.
        """

        Library.framework.MaaPostStop(self._handle)
        return Future(0, self._stop_status)

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
                recognizer.c_handle,
                recognizer.c_arg,
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
                action.c_handle,
                action.c_arg,
            )
        )

    @property
    def c_handle(self) -> ctypes.c_void_p:
        """
        Get the C handle of the instance.

        :return: The C handle of the instance.
        """

        return self._handle

    def _status(self, id: int) -> ctypes.c_int32:
        return Library.framework.MaaTaskStatus(self._handle, id)

    def _stop_status(self, id: int) -> ctypes.c_int32:
        return MaaStatusEnum.success if not self.running() else MaaStatusEnum.running

    def _set_task_param(self, id: int, param: Dict) -> bool:
        return Library.framework.MaaSetTaskParam(
            self._handle, id, json.dumps(param).encode("utf-8")
        )

    @staticmethod
    def set_log_dir(path: Union[Path, str]) -> bool:
        """
        Set the log directory.

        :param path: The path to the log directory.
        """
        return bool(
            Library.framework.MaaSetGlobalOption(
                MaaGlobalOptionEnum.LogDir,
                str(path).encode("utf-8"),
                len(path),
            )
        )

    @staticmethod
    def set_save_draw(save_draw: bool) -> bool:
        """
        Set whether to save draw.

        :param save_draw: Whether to save draw.
        """
        cbool = ctypes.c_bool(save_draw)
        return bool(
            Library.framework.MaaSetGlobalOption(
                MaaGlobalOptionEnum.SaveDraw,
                ctypes.pointer(cbool),
                ctypes.sizeof(ctypes.c_bool),
            )
        )

    @staticmethod
    def set_recording(recording: bool) -> bool:
        """
        Set whether to dump all screenshots and actions.

        :param recording: Whether to dump all screenshots and actions.
        """

        cbool = ctypes.c_bool(recording)
        return bool(
            Library.framework.MaaSetGlobalOption(
                MaaGlobalOptionEnum.Recording,
                ctypes.pointer(cbool),
                ctypes.sizeof(ctypes.c_bool),
            )
        )

    @staticmethod
    def set_stdout_level(level: MaaLoggingLevelEunm) -> bool:
        """
        Set the level of log output to stdout.

        :param level: The level of log output to stdout.
        """

        cbool = ctypes.c_bool(level)
        return bool(
            Library.framework.MaaSetGlobalOption(
                MaaGlobalOptionEnum.StdoutLevel,
                ctypes.pointer(cbool),
                ctypes.sizeof(MaaLoggingLevel),
            )
        )

    @staticmethod
    def set_show_hit_draw(show_hit_draw: bool) -> bool:
        """
        Set whether to show hit draw.

        :param show_hit_draw: Whether to show hit draw.
        """

        cbool = ctypes.c_bool(show_hit_draw)
        return bool(
            Library.framework.MaaSetGlobalOption(
                MaaGlobalOptionEnum.ShowHitDraw,
                ctypes.pointer(cbool),
                ctypes.sizeof(ctypes.c_bool),
            )
        )

    @staticmethod
    def set_debug_message(debug_message: bool) -> bool:
        """
        Set whether to callback debug message.

        :param debug_message: Whether to callback debug message.
        """

        cbool = ctypes.c_bool(debug_message)
        return bool(
            Library.framework.MaaSetGlobalOption(
                MaaGlobalOptionEnum.DebugMessage,
                ctypes.pointer(cbool),
                ctypes.sizeof(ctypes.c_bool),
            )
        )

    @staticmethod
    def query_recognition_detail(reco_id: int) -> Optional[RecognitionDetail]:
        """
        Query recognition detail.

        :param reco_id: The recognition id.
        :return: The recognition detail.
        """

        hit = MaaBool()
        hit_box = RectBuffer()
        detail_json = StringBuffer()
        draws = ImageListBuffer()
        ret = bool(
            Library.framework.MaaQueryRecognitionDetail(
                reco_id,
                ctypes.pointer(hit),
                hit_box.c_handle,
                detail_json.c_handle,
                draws.c_handle,
            )
        )
        if not ret:
            return None

        return RecognitionDetail(
            reco_id=reco_id,
            hit_box=bool(hit) and hit_box.get() or None,
            detail=json.loads(detail_json.get()),
            draws=draws.get(),
        )

    @staticmethod
    def query_running_detail(run_id: int) -> Optional[RunningDetail]:
        """
        Query running detail.

        :param run_id: The running id.
        :return: The running detail.
        """

        reco_id = MaaRecoId()
        successful = MaaBool()

        ret = bool(
            Library.framework.MaaQueryRunningDetail(
                run_id,
                ctypes.pointer(reco_id),
                ctypes.pointer(successful),
            )
        )

        if not ret:
            return None

        recognition = Instance.query_recognition_detail(reco_id)
        if not recognition:
            return None

        return RunningDetail(
            run_id=run_id,
            recognition=recognition,
            successful=bool(successful),
        )

    _api_properties_initialized: bool = False

    @staticmethod
    def _set_api_properties():
        """
        Set the API properties.
        """
        if Instance._api_properties_initialized:
            return
        Instance._api_properties_initialized = True

        Library.framework.MaaCreate.restype = MaaInstanceHandle
        Library.framework.MaaCreate.argtypes = [
            MaaInstanceCallback,
            MaaCallbackTransparentArg,
        ]

        Library.framework.MaaDestroy.restype = None
        Library.framework.MaaDestroy.argtypes = [MaaInstanceHandle]

        Library.framework.MaaBindResource.restype = MaaBool
        Library.framework.MaaBindResource.argtypes = [
            MaaInstanceHandle,
            MaaResourceHandle,
        ]

        Library.framework.MaaBindController.restype = MaaBool
        Library.framework.MaaBindController.argtypes = [
            MaaInstanceHandle,
            MaaControllerHandle,
        ]

        Library.framework.MaaInited.restype = MaaBool
        Library.framework.MaaInited.argtypes = [MaaInstanceHandle]

        Library.framework.MaaPostTask.restype = MaaId
        Library.framework.MaaPostTask.argtypes = [
            MaaInstanceHandle,
            MaaStringView,
            MaaStringView,
        ]

        Library.framework.MaaSetTaskParam.restype = MaaBool
        Library.framework.MaaSetTaskParam.argtypes = [
            MaaInstanceHandle,
            MaaTaskId,
            MaaStringView,
        ]

        Library.framework.MaaTaskStatus.restype = MaaStatus
        Library.framework.MaaTaskStatus.argtypes = [
            MaaInstanceHandle,
            MaaTaskId,
        ]

        Library.framework.MaaRunning.restype = MaaBool
        Library.framework.MaaRunning.argtypes = [MaaInstanceHandle]

        Library.framework.MaaPostStop.restype = MaaBool
        Library.framework.MaaPostStop.argtypes = [MaaInstanceHandle]

        Library.framework.MaaRegisterCustomRecognizer.restype = MaaBool
        Library.framework.MaaRegisterCustomRecognizer.argtypes = [
            MaaInstanceHandle,
            MaaStringView,
            MaaCustomRecognizerHandle,
            MaaTransparentArg,
        ]

        Library.framework.MaaRegisterCustomAction.restype = MaaBool
        Library.framework.MaaRegisterCustomAction.argtypes = [
            MaaInstanceHandle,
            MaaStringView,
            MaaCustomActionHandle,
            MaaTransparentArg,
        ]

        Library.framework.MaaSetGlobalOption.restype = MaaBool
        Library.framework.MaaSetGlobalOption.argtypes = [
            MaaGlobalOption,
            MaaOptionValue,
            MaaOptionValueSize,
        ]

        Library.framework.MaaQueryRecognitionDetail.restype = MaaBool
        Library.framework.MaaQueryRecognitionDetail.argtypes = [
            MaaRecoId,
            ctypes.POINTER(MaaBool),
            MaaRectHandle,
            MaaStringBufferHandle,
            MaaImageListBufferHandle,
        ]

        Library.framework.MaaQueryRunningDetail.restype = MaaBool
        Library.framework.MaaQueryRunningDetail.argtypes = [
            MaaRunningId,
            ctypes.POINTER(MaaRecoId),
            ctypes.POINTER(MaaBool),
        ]


class TaskFuture(Future):
    def __init__(self, maaid: MaaId, status_func, set_param_func):
        super().__init__(maaid, status_func)
        self._set_param_func = set_param_func

    def set_param(self, param: Any) -> bool:
        """
        Set the param of the task.

        :param param: The param of the task.
        :return: True if the param was successfully set, False otherwise.
        """

        return self._set_param_func(self._mid, param)
