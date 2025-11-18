import ctypes
import json
from typing import Any, Dict, Optional, Tuple

import numpy

from .event_sink import EventSink, NotificationType
from .buffer import ImageBuffer, RectBuffer, StringBuffer, StringListBuffer
from .define import *
from .library import Library
from .tasker import Tasker
from .pipeline import JPipelineData, JPipelineParser
from .job import JobWithResult


class Context:
    _handle: MaaContextHandle
    _tasker: Tasker

    ### public ###

    def __init__(self, handle: MaaContextHandle):
        self._set_api_properties()

        self._handle = handle
        if not self._handle:
            raise ValueError("handle is None")

        self._init_tasker()

    def __del__(self):
        pass

    def run_task(
        self, entry: str, pipeline_override: Dict = {}
    ) -> Optional[TaskDetail]:
        """同步执行任务 / Synchronously execute task

        Args:
            entry: 任务入口 / Task entry
            pipeline_override: 用于覆盖的 json / JSON for overriding

        Returns:
            Optional[TaskDetail]: 任务详情，执行失败则返回 None / Task detail, or None if execution failed
        """
        task_id = int(
            Library.framework().MaaContextRunTask(
                self._handle, *Context._gen_post_param(entry, pipeline_override)
            )
        )
        if not task_id:
            return None

        return self.tasker.get_task_detail(task_id)

    def run_recognition(
        self,
        entry: str,
        image: numpy.ndarray,
        pipeline_override: Dict = {},
    ) -> Optional[RecognitionDetail]:
        """同步执行识别逻辑 / Synchronously execute recognition logic

        不会执行后续操作, 不会执行后续 next
        Will not execute subsequent operations or next steps

        Args:
            entry: 任务名 / Task name
            image: 前序截图 / Previous screenshot
            pipeline_override: 用于覆盖的 json / JSON for overriding

        Returns:
            Optional[RecognitionDetail]: 识别结果。无论是否命中，只要尝试进行了识别，就会返回；
            请通过 RecognitionDetail.hit 判断是否命中。只在未能启动识别流程时（如 entry 不存在、node disabled、image 为空等），才可能返回 None。
            Recognition detail. It always returns as long as recognition was attempted;
            use RecognitionDetail.hit to determine hit. Only return None if the recognition process fails to start
            (e.g., entry does not exist, node is disabled, image is empty).
        """
        image_buffer = ImageBuffer()
        image_buffer.set(image)
        reco_id = int(
            Library.framework().MaaContextRunRecognition(
                self._handle,
                *Context._gen_post_param(entry, pipeline_override),
                image_buffer._handle
            )
        )
        if not reco_id:
            return None

        return self.tasker.get_recognition_detail(reco_id)

    def run_action(
        self,
        entry: str,
        box: RectType = (0, 0, 0, 0),
        reco_detail: str = "",
        pipeline_override: Dict = {},
    ) -> Optional[ActionDetail]:
        """同步执行操作逻辑 / Synchronously execute action logic

        不会执行后续 next
        Will not execute subsequent next steps

        Args:
            entry: 任务名 / Task name
            box: 前序识别位置 / Previous recognition position
            reco_detail: 前序识别详情 / Previous recognition details
            pipeline_override: 用于覆盖的 json / JSON for overriding

        Returns:
            Optional[ActionDetail]: 操作结果。无论动作是否成功，只要尝试执行了动作，就会返回；
            请通过 ActionDetail.success 判断是否执行成功。只在未能启动动作流程时（如 entry 不存在、node disabled 等），才可能返回 None。
            Action detail. It always returns as long as the action was attempted;
            use ActionDetail.success to determine success. Only return None if the action flow fails to start
            (e.g., entry does not exist, node is disabled, etc.).
        """
        rect = RectBuffer()
        rect.set(box)

        act_id = int(
            Library.framework().MaaContextRunAction(
                self._handle,
                *Context._gen_post_param(entry, pipeline_override),
                rect._handle,
                reco_detail.encode()
            )
        )

        if not act_id:
            return None

        return self.tasker.get_action_detail(act_id)

    def override_pipeline(self, pipeline_override: Dict) -> bool:
        """覆盖 pipeline / Override pipeline_override

        Args:
            pipeline_override: 用于覆盖的 json / JSON for overriding

        Returns:
            bool: 是否成功 / Whether successful
        """
        pipeline_json = json.dumps(pipeline_override, ensure_ascii=False)

        return bool(
            Library.framework().MaaContextOverridePipeline(
                self._handle,
                pipeline_json.encode(),
            )
        )

    def override_next(self, name: str, next_list: List[str]) -> bool:
        """覆盖任务的 next 列表 / Override the next list of task

        如果节点不存在，此方法会失败
        This method will fail if the node does not exist

        Args:
            name: 任务名 / Task name
            next_list: next 列表 / Next list

        Returns:
            bool: 成功返回 True，如果节点不存在则返回 False / Returns True on success, False if node does not exist
        """
        list_buffer = StringListBuffer()
        list_buffer.set(next_list)

        return bool(
            Library.framework().MaaContextOverrideNext(
                self._handle, name.encode(), list_buffer._handle
            )
        )

    def override_image(self, image_name: str, image: numpy.ndarray) -> bool:
        """覆盖图片 / Override the image corresponding to image_name

        Args:
            image_name: 图片名 / Image name
            image: 图片数据 / Image data

        Returns:
            bool: 是否成功 / Whether successful
        """
        image_buffer = ImageBuffer()
        image_buffer.set(image)

        return bool(
            Library.framework().MaaContextOverrideImage(
                self._handle, image_name.encode(), image_buffer._handle
            )
        )

    def get_node_data(self, name: str) -> Optional[Dict]:
        """获取任务当前的定义 / Get the current definition of task

        Args:
            name: 任务名 / Task name

        Returns:
            Optional[Dict]: 任务定义字典，如果不存在则返回 None / Task definition dict, or None if not exists
        """
        string_buffer = StringBuffer()
        if not Library.framework().MaaContextGetNodeData(
            self._handle, name.encode(), string_buffer._handle
        ):
            return None

        data = string_buffer.get()
        if not data:
            return None

        try:
            return json.loads(data)
        except json.JSONDecodeError:
            return None

    def get_node_object(self, name: str) -> Optional[JPipelineData]:
        node_data = self.get_node_data(name)

        if not node_data:
            return None

        return JPipelineParser.parse_pipeline_data(node_data)

    @property
    def tasker(self) -> Tasker:
        """获取实例 / Get instance

        Returns:
            Tasker: 实例对象 / Instance object
        """
        return self._tasker

    def get_task_job(self) -> JobWithResult:
        """获取对应任务号的任务作业 / Get task job for corresponding task id

        Returns:
            JobWithResult: 任务作业对象 / Task job object

        Raises:
            ValueError: 如果任务 id 为 None
        """
        task_id = Library.framework().MaaContextGetTaskId(self._handle)
        if not task_id:
            raise ValueError("task_id is None")

        return self.tasker._gen_task_job(task_id)

    def clone(self) -> "Context":
        """复制上下文 / Clone context

        Returns:
            Context: 复制的上下文对象 / Cloned context object

        Raises:
            ValueError: 如果克隆失败
        """
        cloned_handle = Library.framework().MaaContextClone(self._handle)
        if not cloned_handle:
            raise ValueError("cloned_handle is None")

        return Context(cloned_handle)

    ### private ###

    def _init_tasker(self):
        tasker_handle = Library.framework().MaaContextGetTasker(self._handle)
        if not tasker_handle:
            raise ValueError("tasker_handle is None")
        self._tasker = Tasker(handle=tasker_handle)

    @staticmethod
    def _gen_post_param(entry: str, pipeline_override: Dict) -> Tuple[bytes, bytes]:
        pipeline_json = json.dumps(pipeline_override, ensure_ascii=False)

        return (
            entry.encode(),
            pipeline_json.encode(),
        )

    _api_properties_initialized: bool = False

    @staticmethod
    def _set_api_properties():
        if Context._api_properties_initialized:
            return

        Context._api_properties_initialized = True

        Library.framework().MaaContextRunTask.restype = MaaTaskId
        Library.framework().MaaContextRunTask.argtypes = [
            MaaContextHandle,
            ctypes.c_char_p,
            ctypes.c_char_p,
        ]

        Library.framework().MaaContextRunRecognition.restype = MaaRecoId
        Library.framework().MaaContextRunRecognition.argtypes = [
            MaaContextHandle,
            ctypes.c_char_p,
            ctypes.c_char_p,
            MaaImageBufferHandle,
        ]

        Library.framework().MaaContextRunAction.restype = MaaActId
        Library.framework().MaaContextRunAction.argtypes = [
            MaaContextHandle,
            ctypes.c_char_p,
            ctypes.c_char_p,
            MaaRectHandle,
            MaaStringBufferHandle,
        ]

        Library.framework().MaaContextOverridePipeline.restype = MaaBool
        Library.framework().MaaContextOverridePipeline.argtypes = [
            MaaContextHandle,
            ctypes.c_char_p,
        ]

        Library.framework().MaaContextOverrideNext.restype = MaaBool
        Library.framework().MaaContextOverrideNext.argtypes = [
            MaaContextHandle,
            ctypes.c_char_p,
            MaaStringListBufferHandle,
        ]

        Library.framework().MaaContextOverrideImage.restype = MaaBool
        Library.framework().MaaContextOverrideImage.argtypes = [
            MaaContextHandle,
            ctypes.c_char_p,
            MaaImageBufferHandle,
        ]

        Library.framework().MaaContextGetNodeData.restype = MaaBool
        Library.framework().MaaContextGetNodeData.argtypes = [
            MaaContextHandle,
            ctypes.c_char_p,
            MaaStringBufferHandle,
        ]

        Library.framework().MaaContextGetTaskId.restype = MaaTaskId
        Library.framework().MaaContextGetTaskId.argtypes = [
            MaaContextHandle,
        ]

        Library.framework().MaaContextGetTasker.restype = MaaTaskerHandle
        Library.framework().MaaContextGetTasker.argtypes = [
            MaaContextHandle,
        ]

        Library.framework().MaaContextClone.restype = MaaContextHandle
        Library.framework().MaaContextClone.argtypes = [
            MaaContextHandle,
        ]


class ContextEventSink(EventSink):

    @dataclass
    class NodeNextListDetail:
        task_id: int
        name: str
        next_list: list[str]
        focus: Any

    def on_node_next_list(
        self,
        context: Context,
        noti_type: NotificationType,
        detail: NodeNextListDetail,
    ):
        pass

    @dataclass
    class NodeRecognitionDetail:
        task_id: int
        reco_id: int
        name: str
        focus: Any

    def on_node_recognition(
        self,
        context: Context,
        noti_type: NotificationType,
        detail: NodeRecognitionDetail,
    ):
        pass

    @dataclass
    class NodeActionDetail:
        task_id: int
        action_id: int
        name: str
        focus: Any

    def on_node_action(
        self, context: Context, noti_type: NotificationType, detail: NodeActionDetail
    ):
        pass

    @dataclass
    class NodePipelineNodeDetail:
        task_id: int
        node_id: int
        name: str
        focus: Any

    def on_node_pipeline_node(
        self,
        context: Context,
        noti_type: NotificationType,
        detail: NodePipelineNodeDetail,
    ):
        pass

    @dataclass
    class NodeRecognitionNodeDetail:
        task_id: int
        node_id: int
        name: str
        focus: Any

    def on_node_recognition_node(
        self,
        context: Context,
        noti_type: NotificationType,
        detail: NodeRecognitionNodeDetail,
    ):
        pass

    @dataclass
    class NodeActionNodeDetail:
        task_id: int
        node_id: int
        name: str
        focus: Any

    def on_node_action_node(
        self,
        context: Context,
        noti_type: NotificationType,
        detail: NodeActionNodeDetail,
    ):
        pass

    def on_raw_notification(self, context: Context, msg: str, details: dict):
        pass

    def _on_raw_notification(self, handle: ctypes.c_void_p, msg: str, details: dict):

        context = Context(handle=handle)
        self.on_raw_notification(context, msg, details)

        noti_type = EventSink._notification_type(msg)
        if msg.startswith("Node.NextList"):
            detail = self.NodeNextListDetail(
                task_id=details["task_id"],
                name=details["name"],
                next_list=details["list"],
                focus=details["focus"],
            )
            self.on_node_next_list(context, noti_type, detail)

        elif msg.startswith("Node.PipelineNode"):
            detail = self.NodePipelineNodeDetail(
                task_id=details["task_id"],
                node_id=details["node_id"],
                name=details["name"],
                focus=details["focus"],
            )
            self.on_node_pipeline_node(context, noti_type, detail)

        elif msg.startswith("Node.RecognitionNode"):
            detail = self.NodeRecognitionNodeDetail(
                task_id=details["task_id"],
                node_id=details["node_id"],
                name=details["name"],
                focus=details["focus"],
            )
            self.on_node_recognition_node(context, noti_type, detail)

        elif msg.startswith("Node.ActionNode"):
            detail = self.NodeActionNodeDetail(
                task_id=details["task_id"],
                node_id=details["node_id"],
                name=details["name"],
                focus=details["focus"],
            )
            self.on_node_action_node(context, noti_type, detail)

        elif msg.startswith("Node.Recognition"):
            detail = self.NodeRecognitionDetail(
                task_id=details["task_id"],
                reco_id=details["reco_id"],
                name=details["name"],
                focus=details["focus"],
            )
            self.on_node_recognition(context, noti_type, detail)

        elif msg.startswith("Node.Action"):
            detail = self.NodeActionDetail(
                task_id=details["task_id"],
                action_id=details["action_id"],
                name=details["name"],
                focus=details["focus"],
            )
            self.on_node_action(context, noti_type, detail)

        else:
            self.on_unknown_notification(context, msg, details)
