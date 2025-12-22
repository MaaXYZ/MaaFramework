import ctypes
from dataclasses import dataclass
from abc import ABC, abstractmethod

from .buffer import RectBuffer
from .context import Context
from .define import *


class CustomAction(ABC):
    """自定义动作基类 / Custom action base class

    用于实现自定义的 Pipeline 动作。继承此类并实现 run 方法，
    然后通过 Resource.register_custom_action 或 AgentServer.register_custom_action 注册。
    Used to implement custom Pipeline actions. Inherit this class and implement the run method,
    then register via Resource.register_custom_action or AgentServer.register_custom_action.

    Example:
        class MyAction(CustomAction):
            def run(self, context, argv):
                context.tasker.controller.post_click(100, 100).wait()
                return True
    """

    _handle: MaaCustomActionCallback

    def __init__(self):
        self._handle = self._c_run_agent

    @dataclass
    class RunArg:
        """run 方法的参数 / Arguments for run method

        Attributes:
            task_detail: 当前任务详情 / Current task detail
            node_name: 当前节点名 / Current node name
            custom_action_name: 自定义动作名 / Custom action name
            custom_action_param: 自定义动作参数 (JSON 字符串) / Custom action parameter (JSON string)
            reco_detail: 前序识别详情 / Previous recognition detail
            box: 前序识别位置 / Previous recognition box
        """

        task_detail: TaskDetail
        node_name: str
        custom_action_name: str
        custom_action_param: str
        reco_detail: RecognitionDetail
        box: Rect

    @dataclass
    class RunResult:
        """run 方法的返回结果 / Return result of run method

        Attributes:
            success: 动作是否执行成功 / Whether the action executed successfully
        """

        success: bool

    @abstractmethod
    def run(
        self,
        context: Context,
        argv: RunArg,
    ) -> Union[RunResult, bool]:
        """执行自定义动作 / Execute custom action

        Args:
            context: 任务上下文，可用于执行其他操作 / Task context, can be used to execute other operations
            argv: 动作参数 / Action arguments

        Returns:
            Union[RunResult, bool]: 执行结果，可返回 RunResult 或 bool / Execution result, can return RunResult or bool
        """
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

        elif result is None:
            return int(True)

        else:
            raise TypeError(f"Invalid return type: {result!r}")
