import numpy
from abc import ABC, abstractmethod
from typing import Optional, Any

from .define import MaaCustomRecognizer
from .buffer import RectBuffer, StringBuffer


class CustomRecognizer(ABC):
    _handle: MaaCustomRecognizer

    @abstractmethod
    def analyze(
        self,
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
