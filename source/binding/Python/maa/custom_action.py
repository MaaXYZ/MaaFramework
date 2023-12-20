import numpy
import ctypes
from abc import ABC, abstractmethod

from .define import MaaBool, MaaCustomAction
from .buffer import RectBuffer, StringBuffer, ImageBuffer


class CustomAction(ABC):
    _handle: MaaCustomAction

    def __init__(self):
        self._handle = MaaCustomAction(self._c_run_agent, self._c_stop_agent)

    @abstractmethod
    def run(
        self,
        context: ctypes.c_void_p,
        task_name: str,
        custom_param: str,
        cur_box: (int, int, int, int),
        cur_rec_detail: str,
    ) -> bool:
        """
        Run the given action.

        :param context: The context.
        :param task_name: The name of the task.
        :param custom_param: The custom action param from pipeline.
        :param cur_box: The current box.
        :param cur_rec_detail: The current recognition detail.

        :return: return success or not
        """

        raise NotImplementedError

    @abstractmethod
    def stop(
        self,
    ) -> None:
        """
        Stop the given action.

        :return: None
        """

        raise NotImplementedError

    def c_handle(self) -> ctypes.POINTER(MaaCustomAction):
        return ctypes.pointer(self._handle)

    def c_arg(self) -> ctypes.c_void_p:
        return ctypes.c_void_p.from_buffer(ctypes.py_object(self))

    @MaaCustomAction.RunFunc
    def _c_run_agent(
        c_context: ctypes.c_void_p,
        c_task_name: ctypes.c_char_p,
        c_custom_param: ctypes.c_char_p,
        c_cur_box: ctypes.c_void_p,
        c_cur_rec_detail: ctypes.c_char_p,
        c_transparent_arg: ctypes.c_void_p,
    ) -> MaaBool:
        if not c_transparent_arg:
            return

        self: CustomAction = ctypes.cast(c_transparent_arg, ctypes.py_object).value

        task_name = c_task_name.decode("utf-8")
        custom_param = c_custom_param.decode("utf-8")

        cur_box = RectBuffer(c_cur_box).get()
        cur_rec_detail = StringBuffer(c_cur_rec_detail).get()

        return self.run(
            c_context,
            task_name,
            custom_param,
            cur_box,
            cur_rec_detail,
        )

    @MaaCustomAction.StopFunc
    def _c_stop_agent(
        c_transparent_arg: ctypes.c_void_p,
    ) -> None:
        if not c_transparent_arg:
            return

        self: CustomAction = ctypes.cast(c_transparent_arg, ctypes.py_object).value

        return self.stop()
