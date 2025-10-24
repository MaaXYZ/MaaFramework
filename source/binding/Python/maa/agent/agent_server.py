import ctypes

from ..define import *
from ..library import Library
from ..event_sink import EventSink


class AgentServer:

    @staticmethod
    def custom_recognition(name: str):

        def wrapper_recognition(recognition):
            AgentServer.register_custom_recognition(
                name=name, recognition=recognition()
            )
            return recognition

        return wrapper_recognition

    _custom_recognition_holder = {}

    @staticmethod
    def register_custom_recognition(
        name: str, recognition: "CustomRecognition"  # type: ignore
    ) -> bool:

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
    def custom_action(name: str):

        def wrapper_action(action):
            AgentServer.register_custom_action(name=name, action=action())
            return action

        return wrapper_action

    _custom_action_holder = {}

    @staticmethod
    def register_custom_action(name: str, action: "CustomAction") -> bool:  # type: ignore

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

        AgentServer._set_api_properties()

        return bool(Library.agent_server().MaaAgentServerStartUp(identifier.encode()))

    @staticmethod
    def shut_down() -> None:

        AgentServer._set_api_properties()

        Library.agent_server().MaaAgentServerShutDown()

    @staticmethod
    def join() -> None:

        AgentServer._set_api_properties()

        Library.agent_server().MaaAgentServerJoin()

    @staticmethod
    def detach() -> None:

        AgentServer._set_api_properties()

        Library.agent_server().MaaAgentServerDetach()

    _sink_holder: List[EventSink] = []

    @staticmethod
    def add_resource_sink(sink: "ResourceEventSink") -> None:
        Library.agent_server().MaaAgentServerAddResourceSink(
            *EventSink._gen_c_param(sink)
        )
        AgentServer._sink_holder.append(sink)

    @staticmethod
    def add_controller_sink(sink: "ControllerEventSink") -> None:
        Library.agent_server().MaaAgentServerAddControllerSink(
            *EventSink._gen_c_param(sink)
        )
        AgentServer._sink_holder.append(sink)

    @staticmethod
    def add_tasker_sink(sink: "TaskerEventSink") -> None:
        Library.agent_server().MaaAgentServerAddTaskerSink(
            *EventSink._gen_c_param(sink)
        )
        AgentServer._sink_holder.append(sink)

    @staticmethod
    def add_tasker_sink(sink: "ContextEventSink") -> None:
        Library.agent_server().MaaAgentServerAddContextSink(
            *EventSink._gen_c_param(sink)
        )
        AgentServer._sink_holder.append(sink)

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

        Library.agent_server().MaaAgentServerAddResourceSink.restype = None
        Library.agent_server().MaaAgentServerAddResourceSink.argtypes = [
            MaaEventCallback,
            ctypes.c_void_p,
        ]

        Library.agent_server().MaaAgentServerAddControllerSink.restype = None
        Library.agent_server().MaaAgentServerAddControllerSink.argtypes = [
            MaaEventCallback,
            ctypes.c_void_p,
        ]

        Library.agent_server().MaaAgentServerAddTaskerSink.restype = None
        Library.agent_server().MaaAgentServerAddTaskerSink.argtypes = [
            MaaEventCallback,
            ctypes.c_void_p,
        ]

        Library.agent_server().MaaAgentServerAddContextSink.restype = None
        Library.agent_server().MaaAgentServerAddContextSink.argtypes = [
            MaaEventCallback,
            ctypes.c_void_p,
        ]
