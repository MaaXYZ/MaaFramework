import numpy
import ctypes
from abc import ABC, abstractmethod

from .define import MaaBool, MaaCustomAction
from .buffer import RectBuffer, StringBuffer, ImageBuffer
from .context import SyncContext

# TODO: Typing

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
        box: (int, int, int, int),
        rec_detail: str,
    ) -> bool:
        """
        Run the given action.

        :param context: The context.
        :param task_name: The name of the task.
        :param custom_param: The custom action param from pipeline.
        :param box: The current box.
        :param rec_detail: The current recognition detail.

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
        c_box: ctypes.c_void_p,
        c_rec_detail: ctypes.c_char_p,
        c_transparent_arg: ctypes.c_void_p,
    ) -> MaaBool:
        if not c_transparent_arg:
            return

        self: CustomAction = ctypes.cast(c_transparent_arg, ctypes.py_object).value

        context = SyncContext(c_context)
        task_name = c_task_name.decode("utf-8")
        custom_param = c_custom_param.decode("utf-8")

        box = RectBuffer(c_box).get()
        rec_detail = c_rec_detail.decode("utf-8")

        return self.run(
            context,
            task_name,
            custom_param,
            box,
            rec_detail,
        )

    @MaaCustomAction.StopFunc
    def _c_stop_agent(
        c_transparent_arg: ctypes.c_void_p,
    ) -> None:
        if not c_transparent_arg:
            return

        self: CustomAction = ctypes.cast(c_transparent_arg, ctypes.py_object).value

        return self.stop()
