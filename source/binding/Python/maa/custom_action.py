import ctypes
from dataclasses import dataclass
from abc import ABC, abstractmethod

from .buffer import RectBuffer
from .context import Context
from .define import *


class CustomAction(ABC):
    _handle: MaaCustomActionCallback

    def __init__(self):
        self._handle = self._c_run_agent

    @dataclass
    class RunArg:
        task_detail: TaskDetail
        node_name: str
        custom_action_name: str
        custom_action_param: str
        reco_detail: RecognitionDetail
        box: Rect

    @dataclass
    class RunResult:
        success: bool

    @abstractmethod
    def run(
        self,
        context: Context,
        argv: RunArg,
    ) -> Union[RunResult, bool]:
        raise NotImplementedError

    @property
    def c_handle(self) -> MaaCustomActionCallback:
        return self._handle

    @property
    def c_arg(self) -> ctypes.c_void_p:
        return ctypes.c_void_p.from_buffer(ctypes.py_object(self))

    @staticmethod
    @MaaCustomActionCallback
    def _c_run_agent(
        c_context: MaaContextHandle,
        c_task_id: MaaTaskId,
        c_node_name: ctypes.c_char_p,
        c_custom_action_name: ctypes.c_char_p,
        c_custom_action_param: ctypes.c_char_p,
        c_reco_id: MaaRecoId,
        c_box: MaaRectHandle,
        c_transparent_arg: ctypes.c_void_p,
    ) -> int:
        if not c_transparent_arg:
            return int(False)

        self: CustomAction = ctypes.cast(
            c_transparent_arg,
            ctypes.py_object,
        ).value

        context = Context(c_context)
        task_detail = context.tasker.get_task_detail(int(c_task_id))
        reco_detail = context.tasker.get_recognition_detail(int(c_reco_id))
        if not task_detail or not reco_detail:
            return int(False)

        box = RectBuffer(c_box).get()

        result: Union[CustomAction.RunResult, bool] = self.run(
            context,
            CustomAction.RunArg(
                task_detail=task_detail,
                node_name=c_node_name.decode(),
                custom_action_name=c_custom_action_name.decode(),
                custom_action_param=c_custom_action_param.decode(),
                reco_detail=reco_detail,
                box=box,
            ),
        )

        if isinstance(result, CustomAction.RunResult):
            return int(result.success)

        elif isinstance(result, bool):
            return int(result)

        else:
            raise TypeError(f"Invalid return type: {result!r}")
