import ctypes
from typing import TYPE_CHECKING, Callable

from ..define import *
from ..event_sink import EventSink
from ..library import Library

if TYPE_CHECKING:
    from ..context import ContextEventSink
    from ..controller import ControllerEventSink
    from ..custom_action import CustomAction
    from ..custom_recognition import CustomRecognition
    from ..resource import ResourceEventSink
    from ..tasker import TaskerEventSink


class AgentServer:
    """Agent 服务端 / Agent server

    用于在独立进程中托管自定义识别器和动作，与 AgentClient 配合使用。
    这允许将复杂的自定义逻辑与主程序分离运行。
    Used to host custom recognitions and actions in a separate process, working with AgentClient.
    This allows separating complex custom logic from the main program.

    Example:
        @AgentServer.custom_recognition("MyReco")
        class MyRecognition(CustomRecognition):
            def analyze(self, context, argv):
                return (100, 100, 50, 50)

        AgentServer.start_up(sock_id)
        AgentServer.join()
    """

    @staticmethod
    def custom_recognition(
        name: str,
    ) -> Callable[[type["CustomRecognition"]], type["CustomRecognition"]]:
        """自定义识别器装饰器 / Custom recognition decorator

        Args:
            name: 识别器名称，需与 Pipeline 中的 custom_recognition 字段匹配
            Recognition name, should match the custom_recognition field in Pipeline

        Returns:
            装饰器函数 / Decorator function
        """

        def wrapper_recognition(
            recognition: type["CustomRecognition"],
        ) -> type["CustomRecognition"]:
            AgentServer.register_custom_recognition(name=name, recognition=recognition())
            return recognition

        return wrapper_recognition

    _custom_recognition_holder: dict[str, "CustomRecognition"] = {}

    @staticmethod
    def register_custom_recognition(
        name: str,
        recognition: "CustomRecognition",
    ) -> bool:
        """注册自定义识别器 / Register custom recognition

        Args:
            name: 识别器名称 / Recognition name
            recognition: 自定义识别器实例 / Custom recognition instance

        Returns:
            bool: 是否成功 / Whether successful
        """

        AgentServer._set_api_properties()

        # avoid gc
        AgentServer._custom_recognition_holder[name] = recognition

        return bool(
            Library.agent_server().MaaAgentServerRegisterCustomRecognition(
                name.encode(),
                recognition.c_handle,
                recognition.c_arg,
            )
        )

    @staticmethod
    def custom_action(
        name: str,
    ) -> Callable[[type["CustomAction"]], type["CustomAction"]]:
        """自定义动作装饰器 / Custom action decorator

        Args:
            name: 动作名称，需与 Pipeline 中的 custom_action 字段匹配
            Action name, should match the custom_action field in Pipeline

        Returns:
            装饰器函数 / Decorator function
        """

        def wrapper_action(action: type["CustomAction"]) -> type["CustomAction"]:
            AgentServer.register_custom_action(name=name, action=action())
            return action

        return wrapper_action

    _custom_action_holder: dict[str, "CustomAction"] = {}

    @staticmethod
    def register_custom_action(name: str, action: "CustomAction") -> bool:
        """注册自定义动作 / Register custom action

        Args:
            name: 动作名称 / Action name
            action: 自定义动作实例 / Custom action instance

        Returns:
            bool: 是否成功 / Whether successful
        """

        AgentServer._set_api_properties()

        # avoid gc
        AgentServer._custom_action_holder[name] = action

        return bool(
            Library.agent_server().MaaAgentServerRegisterCustomAction(
                name.encode(),
                action.c_handle,
                action.c_arg,
            )
        )

    @staticmethod
    def start_up(identifier: str) -> bool:
        """启动 Agent 服务 / Start Agent service

        Args:
            identifier: 连接标识符，用于与 AgentClient 匹配
            Connection identifier for matching with AgentClient

        Returns:
            bool: 是否成功 / Whether successful
        """

        AgentServer._set_api_properties()

        return bool(Library.agent_server().MaaAgentServerStartUp(identifier.encode()))

    @staticmethod
    def shut_down() -> None:
        """关闭 Agent 服务 / Shut down Agent service"""

        AgentServer._set_api_properties()

        Library.agent_server().MaaAgentServerShutDown()

    @staticmethod
    def join() -> None:
        """等待 Agent 服务结束 / Wait for Agent service to end

        阻塞当前线程直到服务结束。
        Blocks the current thread until the service ends.
        """

        AgentServer._set_api_properties()

        Library.agent_server().MaaAgentServerJoin()

    @staticmethod
    def detach() -> None:
        """分离 Agent 服务 / Detach Agent service

        允许服务在后台运行而不阻塞。
        Allows the service to run in the background without blocking.
        """

        AgentServer._set_api_properties()

        Library.agent_server().MaaAgentServerDetach()

    _shutdown_callback_holder: "MaaShutdownCallback | None" = None

    @staticmethod
    def set_shutdown_callback(callback: Callable[[], None]) -> bool:
        """设置关闭回调 / Set shutdown callback

        在 AgentServer 收到 ShutDownRequest 时、停止消息循环前调用；回调返回后服务端才会回复 ShutDownResponse。
        Invoked when the AgentServer receives a ShutDownRequest, before the message loop stops; the ShutDownResponse is replied only after the callback returns.

        回调在服务端消息线程内阻塞执行，须为零参数的同步函数（async 函数传入时只会创建协程、不会真正执行）；
        回调内抛出的异常会被打印到 stderr 后忽略、关闭流程照常继续——清理逻辑失败不会被发现，须自行兜底。
        勿在回调内调用 Join/ShutDown（会等待自身线程，属未定义行为，通常表现为抛异常并终止进程）。
        须在 start_up 前注册（运行中注册会被拒绝），重复注册将覆盖。
        The callback runs blocking on the server message thread, must be a zero-argument synchronous function
        (an async function would only create a coroutine without executing it); exceptions raised inside are
        printed to stderr and ignored while shutdown continues — cleanup failures go unnoticed, so handle them yourself.
        Do not call Join/ShutDown inside (undefined behavior, typically throwing and terminating).
        Must be set before start_up (rejected while the server is running); re-registration overwrites.

        Args:
            callback: 关闭回调函数 / Shutdown callback function

        Returns:
            bool: 是否成功 / Whether successful

        Raises:
            TypeError: callback 不可调用 / callback is not callable
        """

        if not callable(callback):
            raise TypeError(f"callback must be callable, got {type(callback).__name__}")

        AgentServer._set_api_properties()

        @MaaShutdownCallback
        def _c_callback(trans_arg):
            callback()

        # 先在 C++ 侧换指针，成功才替换 holder——两步顺序不能反：
        #   1. 如果先换 holder 再调 C++：失败时（C++ 保留了旧回调 A 的指针），
        #      Python 侧旧回调 A 已被丢弃 → GC 回收 A → C++ 手里是悬垂指针 → 崩溃
        #   2. 现在的顺序：C++ 先换成新回调 B → Python 再丢弃 A 的引用 → 安全
        ret = bool(
            Library.agent_server().MaaAgentServerSetShutdownCallback(
                _c_callback,
                None,
            )
        )

        # 只有注册成功才替换持有引用（防止 GC 把 C++ 还在用的旧回调回收掉）
        if ret:
            AgentServer._shutdown_callback_holder = _c_callback

        return ret

    _sink_holder: dict[int, "EventSink"] = {}

    @staticmethod
    def resource_sink() -> Callable[[type["ResourceEventSink"]], type["ResourceEventSink"]]:
        """资源事件监听器装饰器 / Resource event sink decorator

        Returns:
            装饰器函数 / Decorator function
        """

        def wrapper_sink(
            sink: type["ResourceEventSink"],
        ) -> type["ResourceEventSink"]:
            AgentServer.add_resource_sink(sink=sink())
            return sink

        return wrapper_sink

    @staticmethod
    def add_resource_sink(sink: "ResourceEventSink") -> None:
        """添加资源事件监听器 / Add resource event sink

        Args:
            sink: 资源事件监听器 / Resource event sink
        """
        sink_id = int(Library.agent_server().MaaAgentServerAddResourceSink(*EventSink._gen_c_param(sink)))
        if sink_id == MaaInvalidId:
            return None

        AgentServer._sink_holder[sink_id] = sink

    @staticmethod
    def controller_sink() -> Callable[[type["ControllerEventSink"]], type["ControllerEventSink"]]:
        """控制器事件监听器装饰器 / Controller event sink decorator

        Returns:
            装饰器函数 / Decorator function
        """

        def wrapper_sink(
            sink: type["ControllerEventSink"],
        ) -> type["ControllerEventSink"]:
            AgentServer.add_controller_sink(sink=sink())
            return sink

        return wrapper_sink

    @staticmethod
    def add_controller_sink(sink: "ControllerEventSink") -> None:
        """添加控制器事件监听器 / Add controller event sink

        Args:
            sink: 控制器事件监听器 / Controller event sink
        """
        sink_id = int(Library.agent_server().MaaAgentServerAddControllerSink(*EventSink._gen_c_param(sink)))
        if sink_id == MaaInvalidId:
            return None

        AgentServer._sink_holder[sink_id] = sink

    @staticmethod
    def tasker_sink() -> Callable[[type["TaskerEventSink"]], type["TaskerEventSink"]]:
        """任务器事件监听器装饰器 / Tasker event sink decorator

        Returns:
            装饰器函数 / Decorator function
        """

        def wrapper_sink(sink: type["TaskerEventSink"]) -> type["TaskerEventSink"]:
            AgentServer.add_tasker_sink(sink=sink())
            return sink

        return wrapper_sink

    @staticmethod
    def add_tasker_sink(sink: "TaskerEventSink") -> None:
        """添加任务器事件监听器 / Add tasker event sink

        Args:
            sink: 任务器事件监听器 / Tasker event sink
        """
        sink_id = int(Library.agent_server().MaaAgentServerAddTaskerSink(*EventSink._gen_c_param(sink)))
        if sink_id == MaaInvalidId:
            return None

        AgentServer._sink_holder[sink_id] = sink

    @staticmethod
    def context_sink() -> Callable[[type["ContextEventSink"]], type["ContextEventSink"]]:
        """上下文事件监听器装饰器 / Context event sink decorator

        Returns:
            装饰器函数 / Decorator function
        """

        def wrapper_sink(sink: type["ContextEventSink"]) -> type["ContextEventSink"]:
            AgentServer.add_context_sink(sink=sink())
            return sink

        return wrapper_sink

    @staticmethod
    def add_context_sink(sink: "ContextEventSink") -> None:
        """添加上下文事件监听器 / Add context event sink

        Args:
            sink: 上下文事件监听器 / Context event sink
        """
        sink_id = int(Library.agent_server().MaaAgentServerAddContextSink(*EventSink._gen_c_param(sink)))
        if sink_id == MaaInvalidId:
            return None

        AgentServer._sink_holder[sink_id] = sink

    _api_properties_initialized: bool = False

    @staticmethod
    def _set_api_properties():
        if AgentServer._api_properties_initialized:
            return

        AgentServer._api_properties_initialized = True

        Library.agent_server().MaaAgentServerRegisterCustomRecognition.restype = MaaBool
        Library.agent_server().MaaAgentServerRegisterCustomRecognition.argtypes = [
            ctypes.c_char_p,
            MaaCustomRecognitionCallback,
            ctypes.c_void_p,
        ]

        Library.agent_server().MaaAgentServerRegisterCustomAction.restype = MaaBool
        Library.agent_server().MaaAgentServerRegisterCustomAction.argtypes = [
            ctypes.c_char_p,
            MaaCustomActionCallback,
            ctypes.c_void_p,
        ]

        Library.agent_server().MaaAgentServerStartUp.restype = MaaBool
        Library.agent_server().MaaAgentServerStartUp.argtypes = [
            ctypes.c_char_p,
        ]

        Library.agent_server().MaaAgentServerShutDown.restype = None
        Library.agent_server().MaaAgentServerShutDown.argtypes = []

        Library.agent_server().MaaAgentServerJoin.restype = None
        Library.agent_server().MaaAgentServerJoin.argtypes = []

        Library.agent_server().MaaAgentServerDetach.restype = None
        Library.agent_server().MaaAgentServerDetach.argtypes = []

        Library.agent_server().MaaAgentServerAddResourceSink.restype = MaaSinkId
        Library.agent_server().MaaAgentServerAddResourceSink.argtypes = [
            MaaEventCallback,
            ctypes.c_void_p,
        ]

        Library.agent_server().MaaAgentServerAddControllerSink.restype = MaaSinkId
        Library.agent_server().MaaAgentServerAddControllerSink.argtypes = [
            MaaEventCallback,
            ctypes.c_void_p,
        ]

        Library.agent_server().MaaAgentServerAddTaskerSink.restype = MaaSinkId
        Library.agent_server().MaaAgentServerAddTaskerSink.argtypes = [
            MaaEventCallback,
            ctypes.c_void_p,
        ]

        Library.agent_server().MaaAgentServerAddContextSink.restype = MaaSinkId
        Library.agent_server().MaaAgentServerAddContextSink.argtypes = [
            MaaEventCallback,
            ctypes.c_void_p,
        ]

        Library.agent_server().MaaAgentServerSetShutdownCallback.restype = MaaBool
        Library.agent_server().MaaAgentServerSetShutdownCallback.argtypes = [
            MaaShutdownCallback,
            ctypes.c_void_p,
        ]
