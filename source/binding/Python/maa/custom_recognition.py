import ctypes
import json
from dataclasses import dataclass
from abc import ABC, abstractmethod

import numpy

from .buffer import ImageBuffer, RectBuffer, StringBuffer
from .context import Context
from .define import *


class CustomRecognition(ABC):
    _handle: MaaCustomRecognitionCallback

    def __init__(self):
        self._handle = self._c_analyze_agent

    @dataclass
    class AnalyzeArg:
        task_detail: TaskDetail
        node_name: str
        custom_recognition_name: str
        custom_recognition_param: str
        image: numpy.ndarray
        roi: Rect

    @dataclass
    class AnalyzeResult:
        box: Optional[RectType]
        detail: dict

    @abstractmethod
    def analyze(
        self,
        context: Context,
        argv: AnalyzeArg,
    ) -> Union[AnalyzeResult, Optional[RectType]]:
        raise NotImplementedError

    @property
    def c_handle(self) -> MaaCustomRecognitionCallback:
        return self._handle

    @property
    def c_arg(self) -> ctypes.c_void_p:
        return ctypes.c_void_p.from_buffer(ctypes.py_object(self))

    @staticmethod
    @MaaCustomRecognitionCallback
    def _c_analyze_agent(
        c_context: MaaContextHandle,
        c_task_id: MaaTaskId,
        c_node_name: ctypes.c_char_p,
        c_custom_reco_name: ctypes.c_char_p,
        c_custom_reco_param: ctypes.c_char_p,
        c_image: MaaImageBufferHandle,
        c_roi: MaaRectHandle,
        c_transparent_arg: ctypes.c_void_p,
        c_out_box: MaaRectHandle,
        c_out_detail: MaaStringBufferHandle,
    ) -> int:
        if not c_transparent_arg:
            return int(False)

        self: CustomRecognition = ctypes.cast(c_transparent_arg, ctypes.py_object).value

        context = Context(c_context)
        task_detail = context.tasker.get_task_detail(int(c_task_id))
        if not task_detail:
            return int(False)

        image = ImageBuffer(c_image).get()

        result: Union[CustomRecognition.AnalyzeResult, Optional[RectType]] = (
            self.analyze(
                context,
                CustomRecognition.AnalyzeArg(
                    task_detail=task_detail,
                    node_name=c_node_name.decode(),
                    custom_recognition_name=c_custom_reco_name.decode(),
                    custom_recognition_param=c_custom_reco_param.decode(),
                    image=image,
                    roi=RectBuffer(c_roi).get(),
                ),
            )
        )

        rect_buffer = RectBuffer(c_out_box)
        detail_buffer = StringBuffer(c_out_detail)

        if isinstance(result, CustomRecognition.AnalyzeResult):
            if result.box:
                rect_buffer.set(result.box)
            detail_buffer.set(json.dumps(result.detail, ensure_ascii=False))
            return int(result.box is not None)

        # RectType
        elif (
            isinstance(result, Rect)
            or (
                isinstance(result, list)
                and len(result) == 4
                and all(isinstance(x, int) for x in result)
            )
            or (isinstance(result, numpy.ndarray) and result.size == 4)
            or (
                isinstance(result, tuple)
                and len(result) == 4
                and all(isinstance(x, int) for x in result)
            )
        ):
            rect_buffer.set(result)
            return int(True)

        elif result is None:
            return int(False)

        else:
            raise TypeError(f"Invalid return type: {result!r}")
