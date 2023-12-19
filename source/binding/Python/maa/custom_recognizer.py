import numpy
import ctypes
from abc import ABC, abstractmethod

from .define import MaaBool, MaaCustomRecognizer
from .buffer import RectBuffer, StringBuffer, ImageBuffer


class CustomRecognizer(ABC):
    _handle: MaaCustomRecognizer

    def __init__(self):
        self._handle = MaaCustomRecognizer(self._c_analyze_agent)

    @abstractmethod
    def analyze(
        self,
        context: ctypes.c_void_p,
        image: numpy.ndarray,
        task_name: str,
        custom_recognition_param: str,
    ) -> (bool, (int, int, int, int), str):
        """
        Analyze the given image.

        :param image: The image to analyze.
        :param task_name: The name of the task.
        :param custom_recognition_param: The custom recognition param.

        :return: return a tuple (success, box, detail result)
        """

        raise NotImplementedError

    def c_handle(self) -> ctypes.POINTER(MaaCustomRecognizer):
        return ctypes.pointer(self._handle)

    def c_arg(self) -> ctypes.c_void_p:
        return ctypes.c_void_p.from_buffer(ctypes.py_object(self))

    @MaaCustomRecognizer.AnalyzeFunc
    def _c_analyze_agent(
        c_context: ctypes.c_void_p,
        c_image: ctypes.c_void_p,
        c_task_name: ctypes.c_char_p,
        c_custom_recognition_param: ctypes.c_char_p,
        c_transparent_arg: ctypes.c_void_p,
        c_out_box: ctypes.c_void_p,
        c_out_detail: ctypes.c_void_p,
    ) -> MaaBool:
        if not c_transparent_arg:
            return

        self: CustomRecognizer = ctypes.cast(c_transparent_arg, ctypes.py_object).value

        image = ImageBuffer(c_image).get()
        task_name = c_task_name.decode("utf-8")
        custom_recognition_param = c_custom_recognition_param.decode("utf-8")

        success, box, detail = self.analyze(
            c_context, image, task_name, custom_recognition_param
        )
        RectBuffer(c_out_box).set(box)
        StringBuffer(c_out_detail).set(detail)

        return success
