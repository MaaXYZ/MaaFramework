import ctypes
import json
from typing import Dict, Optional, Tuple

import numpy

from .buffer import ImageBuffer, RectBuffer, StringListBuffer
from .define import *
from .library import Library
from .tasker import Tasker
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
        task_id = int(
            Library.framework().MaaContextRunTask(
                self._handle, *Context._gen_post_param(entry, pipeline_override)
            )
        )
        if not task_id:
            return None

        return self.tasker.get_task_detail(task_id)

    def run_recognition(
        self, entry: str, image: numpy.ndarray, pipeline_override: Dict = {}
    ) -> Optional[RecognitionDetail]:
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
    ) -> Optional[NodeDetail]:
        rect = RectBuffer()
        rect.set(box)

        node_id = int(
            Library.framework().MaaContextRunAction(
                self._handle,
                *Context._gen_post_param(entry, pipeline_override),
                rect._handle,
                reco_detail.encode()
            )
        )

        if not node_id:
            return None

        return self.tasker.get_node_detail(node_id)

    def override_pipeline(self, pipeline_override: Dict) -> bool:
        return bool(
            Library.framework().MaaContextOverridePipeline(
                self._handle,
                json.dumps(pipeline_override, ensure_ascii=False).encode(),
            )
        )

    def override_next(self, name: str, next_list: List[str]) -> bool:
        list_buffer = StringListBuffer()
        list_buffer.set(next_list)

        return bool(
            Library.framework().MaaContextOverrideNext(
                self._handle, name.encode(), list_buffer._handle
            )
        )

    @property
    def tasker(self) -> Tasker:
        return self._tasker

    def get_task_job(self) -> JobWithResult:
        task_id = Library.framework().MaaContextGetTaskId(self._handle)
        if not task_id:
            raise ValueError("task_id is None")

        return self.tasker._gen_task_job(task_id)

    def clone(self) -> "Context":
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
        return (
            entry.encode(),
            json.dumps(pipeline_override, ensure_ascii=False).encode(),
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

        Library.framework().MaaContextRunAction.restype = MaaNodeId
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
