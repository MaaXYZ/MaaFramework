import ctypes
import json
from dataclasses import dataclass
from abc import ABC, abstractmethod

import numpy

from .buffer import ImageBuffer, RectBuffer, StringBuffer
from .context import Context
from .define import *


class CustomRecognition(ABC):
    """自定义识别器基类 / Custom recognition base class

    用于实现自定义的 Pipeline 识别算法。继承此类并实现 analyze 方法，
    然后通过 Resource.register_custom_recognition 或 AgentServer.register_custom_recognition 注册。
    Used to implement custom Pipeline recognition algorithms. Inherit this class and implement the analyze method,
    then register via Resource.register_custom_recognition or AgentServer.register_custom_recognition.

    Example:
        class MyRecognition(CustomRecognition):
            def analyze(self, context, argv):
                # 返回识别到的位置，或 None 表示未识别到
                return (100, 100, 50, 50)
    """

    _handle: MaaCustomRecognitionCallback

    def __init__(self):
        self._handle = self._c_analyze_agent

    @dataclass
    class AnalyzeArg:
        """analyze 方法的参数 / Arguments for analyze method

        Attributes:
            task_detail: 当前任务详情 / Current task detail
            node_name: 当前节点名 / Current node name
            custom_recognition_name: 自定义识别器名 / Custom recognition name
            custom_recognition_param: 自定义识别器参数 (JSON 字符串) / Custom recognition parameter (JSON string)
            image: 待识别的图像 (BGR 格式) / Image to recognize (BGR format)
            roi: 识别区域 / Recognition region of interest
        """

        task_detail: TaskDetail
        node_name: str
        custom_recognition_name: str
        custom_recognition_param: str
        image: numpy.ndarray
        roi: Rect

    @dataclass
    class AnalyzeResult:
        """analyze 方法的返回结果 / Return result of analyze method

        Attributes:
            box: 识别到的位置，None 表示未识别到 / Recognized position, None means not recognized
            detail: 识别详情，会被记录到识别结果中 / Recognition details, will be recorded in recognition result
        """

        box: Optional[RectType]
        detail: dict

    @abstractmethod
    def analyze(
        self,
        context: Context,
        argv: AnalyzeArg,
    ) -> Union[AnalyzeResult, Optional[RectType]]:
        """执行自定义识别 / Execute custom recognition

        Args:
            context: 任务上下文，可用于执行其他操作 / Task context, can be used to execute other operations
            argv: 识别参数 / Recognition arguments

        Returns:
            Union[AnalyzeResult, Optional[RectType]]: 识别结果。可返回 AnalyzeResult、RectType 或 None。
            返回 None 表示未识别到。
            Recognition result. Can return AnalyzeResult, RectType, or None.
            Return None means not recognized.
        """
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
