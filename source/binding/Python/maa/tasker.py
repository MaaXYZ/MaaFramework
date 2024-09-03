import ctypes
import json
import time
from pathlib import Path
from typing import Any, Dict, Optional, Union

from .define import *
from .library import Library
from .buffer import ImageListBuffer, RectBuffer, StringBuffer, ImageBuffer
from .job import Job, MaaStatusEnum
from .callback_agent import Callback, CallbackAgent
from .controller import Controller
from .resource import Resource


@dataclass
class RecognitionDetail:
    reco_id: int
    name: str
    box: Optional[Rect]
    detail: Dict
    raw: numpy.ndarray  # only valid in debug mode
    draws: List[numpy.ndarray]  # only valid in debug mode


@dataclass
class NodeDetail:
    node_id: int
    name: str
    recognition: RecognitionDetail
    times: int
    completed: bool


@dataclass
class TaskDetail:
    task_id: int
    entry: str
    nodes: List[NodeDetail]


class TaskJob(Job):
    def __init__(
        self, maaid: MaaId, status_func, wait_func, override_pp_func, get_detail_func
    ):
        super().__init__(maaid, status_func, wait_func)
        self._pp_override_func = override_pp_func
        self._get_detail_func = get_detail_func

    def override_pipeline(self, pipeline_override: Dict) -> bool:
        return self._pp_override_func(self._maaid, pipeline_override)

    def get(self) -> Optional[TaskDetail]:
        return self._get_detail_func(self._maaid)


class Tasker:
    _callback_agent: CallbackAgent
    _handle: MaaTaskerHandle
    _own: bool = False

    ### public ###

    def __init__(self, callback: Optional[Callback] = None, callback_arg: Any = None, handle: MaaTaskerHandle = None):
        if not Library.initialized:
            raise RuntimeError(
                "Library not initialized, please call `library.open()` first."
            )

        self._set_api_properties()

        self._callback_agent = CallbackAgent(callback, callback_arg)

        if handle:
            self._handle = handle
            self._own = False
        else:
            self._handle = Library.framework.MaaTaskerCreate(
                self._callback_agent.c_callback, self._callback_agent.c_callback_arg
            )
            self._own = True

        if not self._handle:
            raise RuntimeError("Failed to create tasker.")

    def __del__(self):
        if self._handle and self._own:
            Library.framework.MaaTaskerDestroy(self._handle)

    def bind(self, resource: Resource, controller: Controller) -> bool:
        # avoid gc
        self._resource = resource
        self._controller = controller

        return bool(
            Library.framework.MaaTaskerBindResource(self._handle, resource._handle)
        ) and bool(
            Library.framework.MaaTaskerBindController(self._handle, controller._handle)
        )

    @property
    def inited(self) -> bool:
        return bool(Library.framework.MaaTaskerInited(self._handle))

    def post_pipeline(self, entry: str, pipeline_override: Dict = {}) -> TaskJob:
        taskid = Library.framework.MaaTaskerPostPipeline(
            self._handle,
            *Tasker._gen_post_param(entry, pipeline_override),
        )
        return self._gen_task_job(taskid)

    def post_recognition(self, entry: str, pipeline_override: Dict = {}) -> TaskJob:
        taskid = Library.framework.MaaPostRecognition(
            self._handle,
            *Tasker._gen_post_param(entry, pipeline_override),
        )
        return self._gen_task_job(taskid)

    def post_action(self, entry: str, pipeline_override: Dict = {}) -> TaskJob:
        taskid = Library.framework.MaaPostAction(
            self._handle,
            *Tasker._gen_post_param(entry, pipeline_override),
        )
        return self._gen_task_job(taskid)

    def running(self) -> bool:
        return bool(Library.framework.MaaTaskerRunning(self._handle))

    def post_stop(self) -> Job:
        Library.framework.MaaTaskerPostStop(self._handle)
        return Job(0, self._stop_status, self._stop_wait)
    
    def get_latest_node(self, name: str) -> Optional[NodeDetail]:
        node_id = MaaNodeId()
        ret = bool(
            Library.framework.MaaTaskerGetLatestNode(
                self._handle,
                name.encode("utf-8"),
                ctypes.pointer(node_id),
            )
        )
        if not ret:
            return None

        return self._get_node_detail(node_id)

    @staticmethod
    def set_log_dir(path: Union[Path, str]) -> bool:
        return bool(
            Library.framework.MaaSetGlobalOption(
                MaaGlobalOptionEnum.LogDir,
                str(path).encode("utf-8"),
                len(path),
            )
        )

    @staticmethod
    def set_save_draw(save_draw: bool) -> bool:
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
        cbool = ctypes.c_bool(debug_message)
        return bool(
            Library.framework.MaaSetGlobalOption(
                MaaGlobalOptionEnum.DebugMessage,
                ctypes.pointer(cbool),
                ctypes.sizeof(ctypes.c_bool),
            )
        )

    ### private ###

    @staticmethod
    def _gen_post_param(
        entry: str, pipeline_override: Dict
    ) -> Tuple[bytes, bytes]:
        return (
            entry.encode("utf-8"),
            json.dumps(pipeline_override, ensure_ascii=False).encode("utf-8"),
        )

    def _gen_task_job(self, taskid: MaaTaskId) -> TaskJob:
        return TaskJob(
            taskid,
            self._task_status,
            self._task_wait,
            self._task_override_pipeline,
            self._get_task_detail,
        )

    def _task_status(self, id: int) -> ctypes.c_int32:
        return Library.framework.MaaTaskerStatus(self._handle, id)

    def _task_wait(self, id: int) -> ctypes.c_int32:
        return Library.framework.MaaTaskerWait(self._handle, id)

    def _task_override_pipeline(self, id: int, param: Dict) -> bool:
        return bool(
            Library.framework.MaaTaskerOverridePipeline(
                self._handle, id, json.dumps(param, ensure_ascii=False).encode("utf-8")
            )
        )

    def _stop_status(self, id: int) -> ctypes.c_int32:
        return MaaStatusEnum.success if not self.running() else MaaStatusEnum.running

    def _stop_wait(self, id: int) -> ctypes.c_int32:
        # TODO: 这个应该由 callback 来处理
        while self.running():
            time.sleep(0.1)
        return MaaStatusEnum.success

    def _get_recognition_detail(self, reco_id: MaaRecoId) -> Optional[RecognitionDetail]:
        name = StringBuffer()
        hit = MaaBool()
        box = RectBuffer()
        detail_json = StringBuffer()
        raw = ImageBuffer()
        draws = ImageListBuffer()
        ret = bool(
            Library.framework.MaaTaskerGetRecognitionDetail(
                self._handle,
                reco_id,
                name._handle,
                ctypes.pointer(hit),
                box._handle,
                detail_json._handle,
                raw._handle,
                draws._handle,
            )
        )
        if not ret:
            return None

        return RecognitionDetail(
            reco_id=reco_id,
            name=name.get(),
            box=bool(hit) and box.get() or None,
            detail=json.loads(detail_json.get()),
            raw=raw.get(),
            draws=draws.get(),
        )

    def _get_node_detail(self, node_id: MaaNodeId) -> Optional[NodeDetail]:
        name = StringBuffer()
        reco_id = MaaRecoId()
        times = MaaSize()
        completed = MaaBool()

        ret = bool(
            Library.framework.MaaQueryNodeDetail(
                self._handle,
                node_id,
                name._handle,
                ctypes.pointer(reco_id),
                ctypes.pointer(times),
                ctypes.pointer(completed),
            )
        )

        if not ret:
            return None

        recognition = Tasker._get_recognition_detail(reco_id.value)
        if not recognition:
            return None

        return NodeDetail(
            node_id=node_id,
            name=name.get(),
            recognition=recognition,
            times=int(times)
            completed=bool(completed),
        )

    def _get_task_detail(self, task_id: MaaTaskId) -> Optional[TaskDetail]:
        size = MaaSize()
        ret = bool(
            Library.framework.MaaTaskerGetTaskDetail(
                self._handle, task_id, None, None, ctypes.pointer(size)
            )
        )
        if not ret:
            return None

        entry = StringBuffer()
        node_id_list = (MaaNodeId * size.value)()
        ret = bool(
            Library.framework.MaaTaskerGetTaskDetail(
                self._handle, task_id, entry._handle, node_id_list, ctypes.pointer(size)
            )
        )
        if not ret:
            return None

        nodes = []
        for i in range(size.value):
            detail = Tasker._get_node_detail(node_id_list[i])
            nodes.append(detail)

        return TaskDetail(task_id=task_id, entry=entry.get(), nodes=nodes)

    _api_properties_initialized: bool = False

    @staticmethod
    def _set_api_properties():
        if Tasker._api_properties_initialized:
            return
        Tasker._api_properties_initialized = True

        Library.framework.MaaTaskerCreate.restype = MaaTaskerHandle
        Library.framework.MaaTaskerCreate.argtypes = [
            MaaNotificationCallback,
            ctypes.c_void_p,
        ]

        Library.framework.MaaTaskerDestroy.restype = None
        Library.framework.MaaTaskerDestroy.argtypes = [MaaTaskerHandle]

        Library.framework.MaaTaskerBindResource.restype = MaaBool
        Library.framework.MaaTaskerBindResource.argtypes = [
            MaaTaskerHandle,
            MaaResourceHandle,
        ]

        Library.framework.MaaTaskerBindController.restype = MaaBool
        Library.framework.MaaTaskerBindController.argtypes = [
            MaaTaskerHandle,
            MaaControllerHandle,
        ]

        Library.framework.MaaTaskerInited.restype = MaaBool
        Library.framework.MaaTaskerInited.argtypes = [MaaTaskerHandle]

        Library.framework.MaaTaskerPostPipeline.restype = MaaId
        Library.framework.MaaTaskerPostPipeline.argtypes = [
            MaaTaskerHandle,
            ctypes.c_char_p,
            ctypes.c_char_p,
        ]

        Library.framework.MaaPostRecognition.restype = MaaId
        Library.framework.MaaPostRecognition.argtypes = [
            MaaTaskerHandle,
            ctypes.c_char_p,
            ctypes.c_char_p,
        ]

        Library.framework.MaaPostAction.restype = MaaId
        Library.framework.MaaPostAction.argtypes = [
            MaaTaskerHandle,
            ctypes.c_char_p,
            ctypes.c_char_p,
        ]

        Library.framework.MaaTaskerOverridePipeline.restype = MaaBool
        Library.framework.MaaTaskerOverridePipeline.argtypes = [
            MaaTaskerHandle,
            MaaTaskId,
            ctypes.c_char_p,
        ]

        Library.framework.MaaTaskerStatus.restype = MaaStatus
        Library.framework.MaaTaskerStatus.argtypes = [
            MaaTaskerHandle,
            MaaTaskId,
        ]

        Library.framework.MaaTaskerWait.restype = MaaStatus
        Library.framework.MaaTaskerWait.argtypes = [
            MaaTaskerHandle,
            MaaTaskId,
        ]

        Library.framework.MaaTaskerRunning.restype = MaaBool
        Library.framework.MaaTaskerRunning.argtypes = [MaaTaskerHandle]

        Library.framework.MaaTaskerPostStop.restype = MaaBool
        Library.framework.MaaTaskerPostStop.argtypes = [MaaTaskerHandle]

        Library.framework.MaaSetGlobalOption.restype = MaaBool
        Library.framework.MaaSetGlobalOption.argtypes = [
            MaaGlobalOption,
            MaaOptionValue,
            MaaOptionValueSize,
        ]

        Library.framework.MaaTaskerGetRecognitionDetail.restype = MaaBool
        Library.framework.MaaTaskerGetRecognitionDetail.argtypes = [
            MaaTaskerHandle,
            MaaRecoId,
            MaaStringBufferHandle,
            ctypes.POINTER(MaaBool),
            MaaRectHandle,
            MaaStringBufferHandle,
            MaaImageBufferHandle,
            MaaImageListBufferHandle,
        ]

        Library.framework.MaaTaskerGetNodeDetail.restype = MaaBool
        Library.framework.MaaTaskerGetNodeDetail.argtypes = [
            MaaTaskerHandle,
            MaaNodeId,
            MaaStringBufferHandle,
            ctypes.POINTER(MaaRecoId),
            ctypes.POINTER(MaaSize),
            ctypes.POINTER(MaaBool),
        ]

        Library.framework.MaaTaskerGetTaskDetail.restype = MaaBool
        Library.framework.MaaTaskerGetTaskDetail.argtypes = [
            MaaTaskerHandle,
            MaaTaskId,
            MaaStringBufferHandle,
            ctypes.POINTER(MaaRecoId),
            ctypes.POINTER(MaaSize),
        ]

        Library.framework.MaaTaskerGetLatestNode.restype = MaaBool
        Library.framework.MaaTaskerGetLatestNode.argtypes = [
            MaaTaskerHandle,
            ctypes.c_char_p,
            ctypes.POINTER(MaaRecoId),
        ]
