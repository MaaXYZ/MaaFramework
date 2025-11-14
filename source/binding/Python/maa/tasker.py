import ctypes
import json
from pathlib import Path
from typing import Dict, Optional, Union

from .define import *
from .library import Library
from .buffer import ImageListBuffer, RectBuffer, StringBuffer, ImageBuffer
from .job import Job, JobWithResult
from .event_sink import EventSink, NotificationType
from .resource import Resource
from .controller import Controller


class Tasker:
    _handle: MaaTaskerHandle
    _own: bool

    ### public ###

    def __init__(
        self,
        notification_handler: None = None,
        handle: Optional[MaaTaskerHandle] = None,
    ):
        """创建实例 / Create instance

        Args:
            notification_handler: 已废弃，请使用 add_sink, add_context_sink 代替 / Deprecated, use add_sink, add_context_sink instead
            handle: 可选的外部句柄 / Optional external handle

        Raises:
            NotImplementedError: 如果提供了 notification_handler
            RuntimeError: 如果创建失败
        """
        if notification_handler:
            raise NotImplementedError(
                "NotificationHandler is deprecated, use add_sink, add_context_sink instead."
            )

        self._set_api_properties()

        if handle:
            self._handle = handle
            self._own = False
        else:
            self._handle = Library.framework().MaaTaskerCreate()
            self._own = True

        if not self._handle:
            raise RuntimeError("Failed to create tasker.")

    def __del__(self):
        if self._handle and self._own:
            Library.framework().MaaTaskerDestroy(self._handle)

    def bind(self, resource: Resource, controller: Controller) -> bool:
        """关联资源和控制器 / Bind resource and controller

        Args:
            resource: 资源对象 / Resource object
            controller: 控制器对象 / Controller object

        Returns:
            bool: 是否成功 / Whether successful
        """
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
        """获取关联的资源 / Get bound resource

        Returns:
            Resource: 资源对象 / Resource object

        Raises:
            RuntimeError: 如果获取失败
        """
        resource_handle = Library.framework().MaaTaskerGetResource(self._handle)
        if not resource_handle:
            raise RuntimeError("Failed to get resource.")

        return Resource(handle=resource_handle)

    @property
    def controller(self) -> Controller:
        """获取关联的控制器 / Get bound controller

        Returns:
            Controller: 控制器对象 / Controller object

        Raises:
            RuntimeError: 如果获取失败
        """
        controller_handle = Library.framework().MaaTaskerGetController(self._handle)
        if not controller_handle:
            raise RuntimeError("Failed to get controller.")

        return Controller(handle=controller_handle)

    @property
    def inited(self) -> bool:
        """判断是否正确初始化 / Check if initialized correctly

        Returns:
            bool: 是否已正确初始化 / Whether correctly initialized
        """
        return bool(Library.framework().MaaTaskerInited(self._handle))

    def post_task(self, entry: str, pipeline_override: Dict = {}) -> JobWithResult:
        """异步执行任务 / Asynchronously execute task

        这是一个异步操作，会立即返回一个 Job 对象
        This is an asynchronous operation that immediately returns a Job object

        Args:
            entry: 任务入口 / Task entry
            pipeline_override: 用于覆盖的 json / JSON for overriding

        Returns:
            JobWithResult: 任务作业对象，可通过 status/wait 查询状态，通过 get() 获取结果 / Task job object, can query status via status/wait, get result via get()
        """
        taskid = Library.framework().MaaTaskerPostTask(
            self._handle,
            *Tasker._gen_post_param(entry, pipeline_override),
        )
        return self._gen_task_job(taskid)

    @property
    def running(self) -> bool:
        """判断实例是否还在运行 / Check if instance is still running

        Returns:
            bool: 是否正在运行 / Whether running
        """
        return bool(Library.framework().MaaTaskerRunning(self._handle))

    def post_stop(self) -> Job:
        """异步停止实例 / Asynchronously stop instance

        这是一个异步操作，会立即返回一个 Job 对象
        停止操作会中断当前运行的任务，并停止资源加载和控制器操作
        This is an asynchronous operation that immediately returns a Job object
        The stop operation will interrupt the currently running task and stop resource loading and controller operations

        Returns:
            Job: 作业对象，可通过 status/wait 查询状态 / Job object, can query status via status/wait
        """
        taskid = Library.framework().MaaTaskerPostStop(self._handle)
        return self._gen_task_job(taskid)

    @property
    def stopping(self) -> bool:
        """判断实例是否正在停止中(尚未停止) / Check if instance is stopping (not yet stopped)

        Returns:
            bool: 是否正在停止 / Whether stopping
        """
        return bool(Library.framework().MaaTaskerStopping(self._handle))

    def get_latest_node(self, name: str) -> Optional[NodeDetail]:
        """获取任务的最新节点号 / Get latest node id for task

        Args:
            name: 任务名 / Task name

        Returns:
            Optional[NodeDetail]: 节点详情，如果不存在则返回 None / Node detail, or None if not exists
        """
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
        """清理所有可查询的信息 / Clear all queryable information

        Returns:
            bool: 是否成功 / Whether successful
        """
        return bool(Library.framework().MaaTaskerClearCache(self._handle))

    _sink_holder: Dict[int, "EventSink"] = {}

    def add_sink(self, sink: "TaskerEventSink") -> Optional[int]:
        """添加实例事件监听器 / Add instance event listener

        Args:
            sink: 事件监听器 / Event sink

        Returns:
            Optional[int]: 监听器 id，失败返回 None / Listener id, or None if failed
        """
        sink_id = int(
            Library.framework().MaaTaskerAddSink(
                self._handle, *EventSink._gen_c_param(sink)
            )
        )
        if sink_id == MaaInvalidId:
            return None

        self._sink_holder[sink_id] = sink
        return sink_id

    def remove_sink(self, sink_id: int) -> None:
        """移除实例事件监听器 / Remove instance event listener

        Args:
            sink_id: 监听器 id / Listener id
        """
        Library.framework().MaaTaskerRemoveSink(self._handle, sink_id)
        self._sink_holder.pop(sink_id)

    def clear_sinks(self) -> None:
        """清除所有实例事件监听器 / Clear all instance event listeners"""
        Library.framework().MaaTaskerClearSinks(self._handle)

    def add_context_sink(self, sink: "ContextEventSink") -> Optional[int]:
        """添加上下文事件监听器 / Add context event listener

        Args:
            sink: 上下文事件监听器 / Context event sink

        Returns:
            Optional[int]: 监听器 id，失败返回 None / Listener id, or None if failed
        """
        sink_id = int(
            Library.framework().MaaTaskerAddContextSink(
                self._handle, *EventSink._gen_c_param(sink)
            )
        )
        if sink_id == MaaInvalidId:
            return None

        self._sink_holder[sink_id] = sink
        return sink_id

    def remove_context_sink(self, sink_id: int) -> None:
        """移除上下文事件监听器 / Remove context event listener

        Args:
            sink_id: 监听器 id / Listener id
        """
        Library.framework().MaaTaskerRemoveContextSink(self._handle, sink_id)
        self._sink_holder.pop(sink_id)

    def clear_context_sinks(self) -> None:
        """清除所有上下文事件监听器 / Clear all context event listeners"""
        Library.framework().MaaTaskerClearContextSinks(self._handle)

    ### private ###

    @staticmethod
    def _gen_post_param(entry: str, pipeline_override: Dict) -> Tuple[bytes, bytes]:
        pipeline_json = json.dumps(pipeline_override, ensure_ascii=False)

        return (
            entry.encode(),
            pipeline_json.encode(),
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
        """获取识别信息 / Get recognition info

        Args:
            reco_id: 识别号 / Recognition id

        Returns:
            Optional[RecognitionDetail]: 识别详情，如果不存在则返回 None / Recognition detail, or None if not exists
        """
        name = StringBuffer()
        algorithm = StringBuffer()  # type: ignore
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
            hit=bool(hit),
            box=bool(hit) and box.get() or None,
            all_results=parsed_detail[0],
            filtered_results=parsed_detail[1],
            best_result=parsed_detail[2],
            raw_detail=raw_detail,
            raw_image=raw.get(),
            draw_images=draws.get(),
        )

    def get_action_detail(self, action_id: int) -> Optional[ActionDetail]:
        name = StringBuffer()
        action = StringBuffer()
        box = RectBuffer()
        c_success = MaaBool()
        detail_json = StringBuffer()

        ret = bool(
            Library.framework().MaaTaskerGetActionDetail(
                self._handle,
                MaaActId(action_id),
                name._handle,
                action._handle,
                box._handle,
                ctypes.pointer(c_success),
                detail_json._handle,
            )
        )

        if not ret:
            return None

        raw_detail = json.loads(detail_json.get())
        action_enum: ActionEnum = ActionEnum(action.get())
        parsed_result = Tasker._parse_action_raw_detail(action_enum, raw_detail)

        return ActionDetail(
            action_id=action_id,
            name=name.get(),
            action=action_enum,
            box=box.get(),
            success=bool(c_success),
            result=parsed_result,
            raw_detail=raw_detail,
        )

    def get_node_detail(self, node_id: int) -> Optional[NodeDetail]:
        """获取节点信息 / Get node info

        Args:
            node_id: 节点号 / Node id

        Returns:
            Optional[NodeDetail]: 节点详情，如果不存在则返回 None / Node detail, or None if not exists
        """
        name = StringBuffer()
        c_reco_id = MaaRecoId()
        c_action_id = MaaActId()
        c_completed = MaaBool()

        ret = bool(
            Library.framework().MaaTaskerGetNodeDetail(
                self._handle,
                MaaNodeId(node_id),
                name._handle,
                ctypes.pointer(c_reco_id),
                ctypes.pointer(c_action_id),
                ctypes.pointer(c_completed),
            )
        )

        if not ret:
            return None

        recognition = (
            self.get_recognition_detail(int(c_reco_id.value))
            if c_reco_id.value != 0
            else None
        )
        action = (
            self.get_action_detail(int(c_action_id.value))
            if c_action_id.value != 0
            else None
        )

        return NodeDetail(
            node_id=node_id,
            name=name.get(),
            recognition=recognition,
            action=action,
            completed=bool(c_completed),
        )

    def get_task_detail(self, task_id: int) -> Optional[TaskDetail]:
        """获取任务信息 / Get task info

        Args:
            task_id: 任务号 / Task id

        Returns:
            Optional[TaskDetail]: 任务详情，如果不存在则返回 None / Task detail, or None if not exists
        """
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

    @staticmethod
    def set_log_dir(path: Union[Path, str]) -> bool:
        """设置日志路径 / Set the log path

        Args:
            path: 日志路径 / Log path

        Returns:
            bool: 是否成功 / Whether successful
        """
        strpath = str(path)
        return bool(
            Library.framework().MaaGlobalSetOption(
                MaaOption(MaaGlobalOptionEnum.LogDir),
                strpath.encode(),
                len(strpath),
            )
        )

    @staticmethod
    def set_save_draw(save_draw: bool) -> bool:
        """设置是否将识别保存到日志路径/vision中 / Set whether to save recognition results to log path/vision

        开启后 RecoDetail 将可以获取到 draws / When enabled, RecoDetail can retrieve draws

        Args:
            save_draw: 是否保存 / Whether to save

        Returns:
            bool: 是否成功 / Whether successful
        """
        cbool = ctypes.c_bool(save_draw)
        return bool(
            Library.framework().MaaGlobalSetOption(
                MaaOption(MaaGlobalOptionEnum.SaveDraw),
                ctypes.pointer(cbool),
                ctypes.sizeof(ctypes.c_bool),
            )
        )

    @staticmethod
    def set_recording(recording: bool) -> bool:
        """
        Deprecated
        """
        return False

    @staticmethod
    def set_stdout_level(level: LoggingLevelEnum) -> bool:
        """设置日志输出到 stdout 中的级别 / Set the log output level to stdout

        Args:
            level: 日志级别 / Logging level

        Returns:
            bool: 是否成功 / Whether successful
        """
        clevel = MaaLoggingLevel(level)
        return bool(
            Library.framework().MaaGlobalSetOption(
                MaaOption(MaaGlobalOptionEnum.StdoutLevel),
                ctypes.pointer(clevel),
                ctypes.sizeof(MaaLoggingLevel),
            )
        )

    @staticmethod
    def set_debug_mode(debug_mode: bool) -> bool:
        """设置是否启用调试模式 / Set whether to enable debug mode

        调试模式下, RecoDetail 将可以获取到 raw/draws; 所有任务都会被视为 focus 而产生回调
        In debug mode, RecoDetail can retrieve raw/draws; all tasks are treated as focus and produce callbacks

        Args:
            debug_mode: 是否启用调试模式 / Whether to enable debug mode

        Returns:
            bool: 是否成功 / Whether successful
        """
        cbool = ctypes.c_bool(debug_mode)
        return bool(
            Library.framework().MaaGlobalSetOption(
                MaaOption(MaaGlobalOptionEnum.DebugMode),
                ctypes.pointer(cbool),
                ctypes.sizeof(ctypes.c_bool),
            )
        )

    @staticmethod
    def load_plugin(path: Union[Path, str]) -> bool:
        """加载插件 / Load plugin

        可以使用完整路径或仅使用名称, 仅使用名称时会在系统目录和当前目录中搜索. 也可以递归搜索目录中的插件
        Can use full path or name only. When using name only, will search in system directory and current directory. Can also recursively search for plugins in a directory

        Args:
            path: 插件库路径或名称 / Plugin library path or name

        Returns:
            bool: 是否成功 / Whether successful
        """
        strpath = str(path)
        return bool(
            Library.framework().MaaGlobalLoadPlugin(
                strpath.encode(),
            )
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
        filtered_results: List[RecognitionResult] = []
        best_result: Optional[RecognitionResult] = None

        raw_all_results = raw_detail.get("all", [])
        raw_filtered_results = raw_detail.get("filtered", [])
        raw_best_result = raw_detail.get("best", None)

        for raw_result in raw_all_results:
            all_results.append(ResultType(**raw_result))
        for raw_result in raw_filtered_results:
            filtered_results.append(ResultType(**raw_result))
        if raw_best_result:
            best_result = ResultType(**raw_best_result)

        return all_results, filtered_results, best_result

    @staticmethod
    def _parse_action_raw_detail(
        action: ActionEnum, raw_detail: Dict
    ) -> Optional[ActionResult]:
        if not raw_detail:
            return None

        ResultType = ActionResultDict[action]
        if not ResultType:
            return None

        try:
            # cv::Point 在 JSON 中是数组 [x, y]，不需要转换
            # 直接使用 raw_detail 创建结果对象
            return ResultType(**raw_detail)
        except (TypeError, KeyError):
            # 如果解析失败，返回 None
            return None

    @staticmethod
    def _set_api_properties():
        if Tasker._api_properties_initialized:
            return
        Tasker._api_properties_initialized = True

        Library.framework().MaaGlobalSetOption.restype = MaaBool
        Library.framework().MaaGlobalSetOption.argtypes = [
            MaaGlobalOption,
            MaaOptionValue,
            MaaOptionValueSize,
        ]

        Library.framework().MaaGlobalLoadPlugin.restype = MaaBool
        Library.framework().MaaGlobalLoadPlugin.argtypes = [
            ctypes.c_char_p,
        ]

        Library.framework().MaaTaskerCreate.restype = MaaTaskerHandle
        Library.framework().MaaTaskerCreate.argtypes = []

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

        Library.framework().MaaTaskerGetActionDetail.restype = MaaBool
        Library.framework().MaaTaskerGetActionDetail.argtypes = [
            MaaTaskerHandle,
            MaaActId,
            MaaStringBufferHandle,
            MaaStringBufferHandle,
            MaaRectHandle,
            ctypes.POINTER(MaaBool),
            MaaStringBufferHandle,
        ]

        Library.framework().MaaTaskerGetNodeDetail.restype = MaaBool
        Library.framework().MaaTaskerGetNodeDetail.argtypes = [
            MaaTaskerHandle,
            MaaNodeId,
            MaaStringBufferHandle,
            ctypes.POINTER(MaaRecoId),
            ctypes.POINTER(MaaActId),
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

        Library.framework().MaaTaskerAddSink.restype = MaaSinkId
        Library.framework().MaaTaskerAddSink.argtypes = [
            MaaTaskerHandle,
            MaaEventCallback,
            ctypes.c_void_p,
        ]

        Library.framework().MaaTaskerRemoveSink.restype = None
        Library.framework().MaaTaskerRemoveSink.argtypes = [
            MaaTaskerHandle,
            MaaSinkId,
        ]

        Library.framework().MaaTaskerClearSinks.restype = None
        Library.framework().MaaTaskerClearSinks.argtypes = [MaaTaskerHandle]

        Library.framework().MaaTaskerAddContextSink.restype = MaaSinkId
        Library.framework().MaaTaskerAddContextSink.argtypes = [
            MaaTaskerHandle,
            MaaEventCallback,
            ctypes.c_void_p,
        ]

        Library.framework().MaaTaskerRemoveContextSink.restype = None
        Library.framework().MaaTaskerRemoveContextSink.argtypes = [
            MaaTaskerHandle,
            MaaSinkId,
        ]

        Library.framework().MaaTaskerClearContextSinks.restype = None
        Library.framework().MaaTaskerClearContextSinks.argtypes = [MaaTaskerHandle]


class TaskerEventSink(EventSink):

    @dataclass
    class TaskerTaskDetail:
        task_id: int
        entry: str
        uuid: str
        hash: str

    def on_tasker_task(
        self, tasker: Tasker, noti_type: NotificationType, detail: TaskerTaskDetail
    ):
        pass

    def on_raw_notification(self, tasker: Tasker, msg: str, details: dict):
        pass

    def _on_raw_notification(self, handle: ctypes.c_void_p, msg: str, details: dict):

        tasker = Tasker(handle=handle)
        self.on_raw_notification(tasker, msg, details)

        noti_type = EventSink._notification_type(msg)
        if msg.startswith("Tasker.Task"):
            detail = self.TaskerTaskDetail(
                task_id=details["task_id"],
                entry=details["entry"],
                uuid=details["uuid"],
                hash=details["hash"],
            )
            self.on_tasker_task(tasker, noti_type, detail)

        else:
            self.on_unknown_notification(tasker, msg, details)
