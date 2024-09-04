import ctypes
from abc import ABC, abstractmethod
from typing import Tuple

import numpy

from .buffer import ImageBuffer, RectBuffer, StringBuffer
from .context import Context
from .define import *
from .tasker import TaskDetail


class CustomRecognizer(ABC):
    _handle: MaaCustomRecognizerCallback

    def __init__(self):
        self._handle = self._c_analyze_agent

    @abstractmethod
    def analyze(
        self,
        context: Context,
        task_detail: TaskDetail,
        action_name: str,
        custom_param: str,
        image: numpy.ndarray,
    ) -> Tuple[bool, RectType, str]:
        raise NotImplementedError

    @property
    def c_handle(self) -> MaaCustomRecognizerCallback:
        return self._handle

    @property
    def c_arg(self) -> ctypes.c_void_p:
        return ctypes.c_void_p.from_buffer(ctypes.py_object(self))

    @MaaCustomRecognizerCallback
    def _c_analyze_agent(
        c_context: MaaContextHandle,
        c_task_id: MaaTaskId,
        c_reco_name: ctypes.c_char_p,
        c_custom_param: ctypes.c_char_p,
        c_image: MaaImageBufferHandle,
        c_transparent_arg: ctypes.c_void_p,
        c_out_box: MaaRectHandle,
        c_out_detail: MaaStringBufferHandle,
    ) -> MaaBool:
        if not c_transparent_arg:
            return

        self: CustomRecognizer = ctypes.cast(c_transparent_arg, ctypes.py_object).value

        context = Context(c_context)
        task_detail = context.tasker()._get_task_detail(c_task_id)

        image = ImageBuffer(c_image).get()

        hit, box, detail = self.analyze(
            context,
            task_detail,
            c_reco_name.decode("utf-8"),
            c_custom_param.decode("utf-8"),
            image,
        )
        RectBuffer(c_out_box).set(box)
        StringBuffer(c_out_detail).set(detail)

        return MaaBool(hit)
