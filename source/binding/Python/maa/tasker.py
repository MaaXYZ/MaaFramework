import ctypes
import json
from pathlib import Path
from typing import Any, Dict, Optional, Union

from .define import *
from .library import Library
from .buffer import ImageListBuffer, RectBuffer, StringBuffer, ImageBuffer
from .job import Job, JobWithResult
from .notification_handler import NotificationHandler
from .resource import Resource
from .controller import Controller


class Tasker:
    _notification_handler: Optional[NotificationHandler]
    _handle: MaaTaskerHandle
    _own: bool

    ### public ###

    def __init__(
        self,
        notification_handler: Optional[NotificationHandler] = None,
        handle: Optional[MaaTaskerHandle] = None,
    ):
        self._set_api_properties()

        if handle:
            self._handle = handle
            self._own = False
        else:
            self._notification_handler = notification_handler
            self._handle = Library.framework().MaaTaskerCreate(
                *NotificationHandler._gen_c_param(self._notification_handler)
            )
            self._own = True

        if not self._handle:
            raise RuntimeError("Failed to create tasker.")

    def __del__(self):
        if self._handle and self._own:
            Library.framework().MaaTaskerDestroy(self._handle)

    def bind(self, resource: Resource, controller: Controller) -> bool:
        # avoid gc
        self._resource_holder = resource
        self._controller_holder = controller

        return bool(
            Library.framework().MaaTaskerBindResource(self._handle, resource._handle)
        ) and bool(
            Library.framework().MaaTaskerBindController(
                self._handle, controller._handle
            )
        )

    @property
    def resource(self) -> Resource:
        resource_handle = Library.framework().MaaTaskerGetResource(self._handle)
        if not resource_handle:
            raise RuntimeError("Failed to get resource.")

        return Resource(handle=resource_handle)

    @property
    def controller(self) -> Controller:
        controller_handle = Library.framework().MaaTaskerGetController(self._handle)
        if not controller_handle:
            raise RuntimeError("Failed to get controller.")

        return Controller(handle=controller_handle)

    @property
    def inited(self) -> bool:
        return bool(Library.framework().MaaTaskerInited(self._handle))

    def post_task(self, entry: str, pipeline_override: Dict = {}) -> JobWithResult:
        taskid = Library.framework().MaaTaskerPostTask(
            self._handle,
            *Tasker._gen_post_param(entry, pipeline_override),
        )
        return self._gen_task_job(taskid)

    @property
    def running(self) -> bool:
        return bool(Library.framework().MaaTaskerRunning(self._handle))

    def post_stop(self) -> Job:
        taskid = Library.framework().MaaTaskerPostStop(self._handle)
        return self._gen_task_job(taskid)

    @property
    def stopping(self) -> bool:
        return bool(Library.framework().MaaTaskerStopping(self._handle))

    def get_latest_node(self, name: str) -> Optional[NodeDetail]:
        c_node_id = MaaNodeId()
        ret = bool(
            Library.framework().MaaTaskerGetLatestNode(
                self._handle,
                name.encode(),
                ctypes.pointer(c_node_id),
            )
        )
        if not ret:
            return None

        return self.get_node_detail(int(c_node_id.value))

    def clear_cache(self) -> bool:
        return bool(Library.framework().MaaTaskerClearCache(self._handle))

    @staticmethod
    def set_log_dir(path: Union[Path, str]) -> bool:
        strpath = str(path)
        return bool(
            Library.framework().MaaSetGlobalOption(
                MaaOption(MaaGlobalOptionEnum.LogDir),
                strpath.encode(),
                len(strpath),
            )
        )

    @staticmethod
    def set_save_draw(save_draw: bool) -> bool:
        cbool = ctypes.c_bool(save_draw)
        return bool(
            Library.framework().MaaSetGlobalOption(
                MaaOption(MaaGlobalOptionEnum.SaveDraw),
                ctypes.pointer(cbool),
                ctypes.sizeof(ctypes.c_bool),
            )
        )

    @staticmethod
    def set_recording(recording: bool) -> bool:
        cbool = ctypes.c_bool(recording)
        return bool(
            Library.framework().MaaSetGlobalOption(
                MaaOption(MaaGlobalOptionEnum.Recording),
                ctypes.pointer(cbool),
                ctypes.sizeof(ctypes.c_bool),
            )
        )

    @staticmethod
    def set_stdout_level(level: LoggingLevelEnum) -> bool:
        clevel = MaaLoggingLevel(level)
        return bool(
            Library.framework().MaaSetGlobalOption(
                MaaOption(MaaGlobalOptionEnum.StdoutLevel),
                ctypes.pointer(clevel),
                ctypes.sizeof(MaaLoggingLevel),
            )
        )

    @staticmethod
    def set_show_hit_draw(show_hit_draw: bool) -> bool:
        cbool = ctypes.c_bool(show_hit_draw)
        return bool(
            Library.framework().MaaSetGlobalOption(
                MaaOption(MaaGlobalOptionEnum.ShowHitDraw),
                ctypes.pointer(cbool),
                ctypes.sizeof(ctypes.c_bool),
            )
        )

    @staticmethod
    def set_debug_mode(debug_mode: bool) -> bool:
        cbool = ctypes.c_bool(debug_mode)
        return bool(
            Library.framework().MaaSetGlobalOption(
                MaaOption(MaaGlobalOptionEnum.DebugMode),
                ctypes.pointer(cbool),
                ctypes.sizeof(ctypes.c_bool),
            )
        )

    ### private ###

    @staticmethod
    def _gen_post_param(entry: str, pipeline_override: Dict) -> Tuple[bytes, bytes]:
        return (
            entry.encode(),
            json.dumps(pipeline_override, ensure_ascii=False).encode(),
        )

    def _gen_task_job(self, taskid: MaaTaskId) -> JobWithResult:
        return JobWithResult(
            taskid,
            self._task_status,
            self._task_wait,
            self.get_task_detail,
        )

    def _task_status(self, id: int) -> ctypes.c_int32:
        return Library.framework().MaaTaskerStatus(self._handle, id)

    def _task_wait(self, id: int) -> ctypes.c_int32:
        return Library.framework().MaaTaskerWait(self._handle, id)

    def get_recognition_detail(self, reco_id: int) -> Optional[RecognitionDetail]:
        name = StringBuffer()
        algorithm = StringBuffer()
        hit = MaaBool()
        box = RectBuffer()
        detail_json = StringBuffer()
        raw = ImageBuffer()
        draws = ImageListBuffer()
        ret = bool(
            Library.framework().MaaTaskerGetRecognitionDetail(
                self._handle,
                MaaRecoId(reco_id),
                name._handle,
                algorithm._handle,
                ctypes.pointer(hit),
                box._handle,
                detail_json._handle,
                raw._handle,
                draws._handle,
            )
        )
        if not ret:
            return None

        raw_detail = json.loads(detail_json.get())
        algorithm: AlgorithmEnum = AlgorithmEnum(algorithm.get())
        parsed_detail = Tasker._parse_recognition_raw_detail(algorithm, raw_detail)

        return RecognitionDetail(
            reco_id=reco_id,
            name=name.get(),
            algorithm=algorithm,
            box=bool(hit) and box.get() or None,
            all_results=parsed_detail[0],
            filterd_results=parsed_detail[1],
            best_result=parsed_detail[2],
            raw_detail=raw_detail,
            raw_image=raw.get(),
            draw_images=draws.get(),
        )

    def get_node_detail(self, node_id: int) -> Optional[NodeDetail]:
        name = StringBuffer()
        c_reco_id = MaaRecoId()
        c_completed = MaaBool()

        ret = bool(
            Library.framework().MaaTaskerGetNodeDetail(
                self._handle,
                MaaNodeId(node_id),
                name._handle,
                ctypes.pointer(c_reco_id),
                ctypes.pointer(c_completed),
            )
        )

        if not ret:
            return None

        recognition = self.get_recognition_detail(int(c_reco_id.value))
        if not recognition:
            return None

        return NodeDetail(
            node_id=node_id,
            name=name.get(),
            recognition=recognition,
            completed=bool(c_completed),
        )

    def get_task_detail(self, task_id: int) -> Optional[TaskDetail]:
        size = MaaSize()
        entry = StringBuffer()
        status = MaaStatus()
        ret = bool(
            Library.framework().MaaTaskerGetTaskDetail(
                self._handle,
                MaaTaskId(task_id),
                entry._handle,
                None,
                ctypes.pointer(size),
                ctypes.pointer(status),
            )
        )
        if not ret:
            return None

        c_node_id_list = (MaaNodeId * size.value)()
        ret = bool(
            Library.framework().MaaTaskerGetTaskDetail(
                self._handle,
                MaaTaskId(task_id),
                entry._handle,
                c_node_id_list,
                ctypes.pointer(size),
                ctypes.pointer(status),
            )
        )
        if not ret:
            return None

        nodes = []
        for i in range(size.value):
            detail = self.get_node_detail(int(c_node_id_list[i]))
            nodes.append(detail)

        return TaskDetail(
            task_id=task_id, entry=entry.get(), nodes=nodes, status=Status(status)
        )

    _api_properties_initialized: bool = False

    @staticmethod
    def _parse_recognition_raw_detail(algorithm: AlgorithmEnum, raw_detail: Dict):
        if not raw_detail:
            return [], [], None

        ResultType = AlgorithmResultDict[algorithm]
        if not ResultType:
            return [], [], None

        all_results: List[RecognitionResult] = []
        filterd_results: List[RecognitionResult] = []
        best_result: Optional[RecognitionResult] = None

        raw_all_results = raw_detail.get("all", [])
        raw_filterd_results = raw_detail.get("filtered", [])
        raw_best_result = raw_detail.get("best", None)

        for raw_result in raw_all_results:
            all_results.append(ResultType(**raw_result))
        for raw_result in raw_filterd_results:
            filterd_results.append(ResultType(**raw_result))
        if raw_best_result:
            best_result = ResultType(**raw_best_result)

        return all_results, filterd_results, best_result

    @staticmethod
    def _set_api_properties():
        if Tasker._api_properties_initialized:
            return
        Tasker._api_properties_initialized = True

        Library.framework().MaaTaskerCreate.restype = MaaTaskerHandle
        Library.framework().MaaTaskerCreate.argtypes = [
            MaaNotificationCallback,
            ctypes.c_void_p,
        ]

        Library.framework().MaaTaskerDestroy.restype = None
        Library.framework().MaaTaskerDestroy.argtypes = [MaaTaskerHandle]

        Library.framework().MaaTaskerBindResource.restype = MaaBool
        Library.framework().MaaTaskerBindResource.argtypes = [
            MaaTaskerHandle,
            MaaResourceHandle,
        ]

        Library.framework().MaaTaskerBindController.restype = MaaBool
        Library.framework().MaaTaskerBindController.argtypes = [
            MaaTaskerHandle,
            MaaControllerHandle,
        ]

        Library.framework().MaaTaskerInited.restype = MaaBool
        Library.framework().MaaTaskerInited.argtypes = [MaaTaskerHandle]

        Library.framework().MaaTaskerPostTask.restype = MaaId
        Library.framework().MaaTaskerPostTask.argtypes = [
            MaaTaskerHandle,
            ctypes.c_char_p,
            ctypes.c_char_p,
        ]

        Library.framework().MaaTaskerStatus.restype = MaaStatus
        Library.framework().MaaTaskerStatus.argtypes = [
            MaaTaskerHandle,
            MaaTaskId,
        ]

        Library.framework().MaaTaskerWait.restype = MaaStatus
        Library.framework().MaaTaskerWait.argtypes = [
            MaaTaskerHandle,
            MaaTaskId,
        ]

        Library.framework().MaaTaskerRunning.restype = MaaBool
        Library.framework().MaaTaskerRunning.argtypes = [MaaTaskerHandle]

        Library.framework().MaaTaskerPostStop.restype = MaaTaskId
        Library.framework().MaaTaskerPostStop.argtypes = [MaaTaskerHandle]

        Library.framework().MaaTaskerStopping.restype = MaaBool
        Library.framework().MaaTaskerStopping.argtypes = [MaaTaskerHandle]

        Library.framework().MaaTaskerGetResource.restype = MaaResourceHandle
        Library.framework().MaaTaskerGetResource.argtypes = [MaaTaskerHandle]

        Library.framework().MaaTaskerGetController.restype = MaaControllerHandle
        Library.framework().MaaTaskerGetController.argtypes = [MaaTaskerHandle]

        Library.framework().MaaTaskerGetRecognitionDetail.restype = MaaBool
        Library.framework().MaaTaskerGetRecognitionDetail.argtypes = [
            MaaTaskerHandle,
            MaaRecoId,
            MaaStringBufferHandle,
            MaaStringBufferHandle,
            ctypes.POINTER(MaaBool),
            MaaRectHandle,
            MaaStringBufferHandle,
            MaaImageBufferHandle,
            MaaImageListBufferHandle,
        ]

        Library.framework().MaaTaskerGetNodeDetail.restype = MaaBool
        Library.framework().MaaTaskerGetNodeDetail.argtypes = [
            MaaTaskerHandle,
            MaaNodeId,
            MaaStringBufferHandle,
            ctypes.POINTER(MaaRecoId),
            ctypes.POINTER(MaaBool),
        ]

        Library.framework().MaaTaskerGetTaskDetail.restype = MaaBool
        Library.framework().MaaTaskerGetTaskDetail.argtypes = [
            MaaTaskerHandle,
            MaaTaskId,
            MaaStringBufferHandle,
            ctypes.POINTER(MaaRecoId),
            ctypes.POINTER(MaaSize),
            ctypes.POINTER(MaaStatus),
        ]

        Library.framework().MaaTaskerGetLatestNode.restype = MaaBool
        Library.framework().MaaTaskerGetLatestNode.argtypes = [
            MaaTaskerHandle,
            ctypes.c_char_p,
            ctypes.POINTER(MaaRecoId),
        ]

        Library.framework().MaaTaskerClearCache.restype = MaaBool
        Library.framework().MaaTaskerClearCache.argtypes = [
            MaaTaskerHandle,
        ]

        Library.framework().MaaSetGlobalOption.restype = MaaBool
        Library.framework().MaaSetGlobalOption.argtypes = [
            MaaGlobalOption,
            MaaOptionValue,
            MaaOptionValueSize,
        ]
