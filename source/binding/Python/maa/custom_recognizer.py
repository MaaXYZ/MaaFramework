import numpy
import ctypes
from abc import ABC, abstractmethod
from typing import Optional, Any

from .define import MaaBool
from .buffer import RectBuffer, StringBuffer, ImageBuffer


class MaaCustomRecognizer(ctypes.Structure):
    AnalyzeAPI = ctypes.CFUNCTYPE(
        MaaBool,
        ctypes.c_void_p,
        ctypes.c_void_p,
        ctypes.c_char_p,
        ctypes.c_char_p,
        ctypes.c_void_p,
        ctypes.c_void_p,
        ctypes.c_void_p,
    )
    _fields_ = [
        ("analyze", AnalyzeAPI),
    ]


class CustomRecognizer(ABC):
    _handle: MaaCustomRecognizer

    def __init__(self):
        self._handle = MaaCustomRecognizer()
        self._handle.analyze = self.c_analyze()

    def c_analyze(self) -> MaaCustomRecognizer.AnalyzeAPI:
        return self._c_analyze

    @MaaCustomRecognizer.AnalyzeAPI
    def _c_analyze(
        self,
        c_context: ctypes.c_void_p,
        c_image: ctypes.c_void_p,
        c_task_name: ctypes.c_char_p,
        c_custom_recognition_param: ctypes.c_char_p,
        c_transparent_arg: ctypes.c_void_p,
        c_out_box: ctypes.c_void_p,
        c_out_detail_result: ctypes.c_void_p,
    ) -> MaaBool:
        image = ImageBuffer(c_image).get()
        task_name = c_task_name.decode("utf-8")
        custom_recognition_param = c_custom_recognition_param.decode("utf-8")

        success, box, detail_result = self.analyze(
            c_context, image, task_name, custom_recognition_param, c_transparent_arg
        )
        RectBuffer(c_out_box).set(box)
        StringBuffer(c_out_detail_result).set(detail_result)

        return MaaBool(success)

    @abstractmethod
    def analyze(
        self,
        context: ctypes.c_void_p,
        image: numpy.ndarray,
        task_name: str,
        custom_recognition_param: str,
        transparent_arg: Any,
    ) -> (bool, (int, int, int, int), str):
        """
        Analyze the given image.

        :param image: The image to analyze.
        :param task_name: The name of the task.
        :param custom_recognition_param: The custom recognition param.
        :param arg: The transparent arg.

        :return: return a tuple (success, box, detail result)
        """

        raise NotImplementedError
