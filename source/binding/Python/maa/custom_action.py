import ctypes
from abc import ABC, abstractmethod

from .buffer import RectBuffer
from .context import Context
from .define import *
from .tasker import TaskDetail


class CustomAction(ABC):
    _handle: MaaCustomActionCallback

    def __init__(self):
        self._handle = self._c_run_agent

    @abstractmethod
    def run(
        self,
        context: Context,
        task_detail: TaskDetail,
        action_name: str,
        custom_action_param: str,
        box: Rect,
        reco_detail: str,
    ) -> bool:
        raise NotImplementedError

    @property
    def c_handle(self) -> MaaCustomActionCallback:
        return self._handle

    @property
    def c_arg(self) -> ctypes.c_void_p:
        return ctypes.c_void_p.from_buffer(ctypes.py_object(self))

    @MaaCustomActionCallback
    def _c_run_agent(
        c_context: MaaContextHandle,
        c_task_id: MaaTaskId,
        c_action_name: ctypes.c_char_p,
        c_custom_param: ctypes.c_char_p,
        c_box: MaaRectHandle,
        c_reco_detail: ctypes.c_char_p,
        c_transparent_arg: ctypes.c_void_p,
    ) -> MaaBool:
        if not c_transparent_arg:
            return

        self: CustomAction = ctypes.cast(
            c_transparent_arg,
            ctypes.py_object,
        ).value

        context = Context(c_context)
        task_detail = context.tasker()._get_task_detail(c_task_id)
        box = RectBuffer(c_box).get()

        return self.run(
            context,
            task_detail,
            c_action_name.decode("utf-8"),
            c_custom_param.decode("utf-8"),
            box,
            c_reco_detail.decode("utf-8"),
        )
