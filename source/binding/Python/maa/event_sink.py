import ctypes
import json
from abc import ABC
from typing import Tuple
from enum import IntEnum

from .define import MaaEventCallback


# class NotificationEvent(IntEnum):
#     ResourceLoading = 1
#     ControllerAction = 2
#     TaskerTask = 3
#     TaskNextList = 4
#     TaskRecognition = 5
#     TaskAction = 6


class NotificationType(IntEnum):
    Unknown = 0
    Starting = 1
    Succeeded = 2
    Failed = 3


class EventSink(ABC):
    def on_unknown_notification(self, instance, msg: str, details: dict):
        pass

    def _on_raw_notification(self, handle: ctypes.c_void_p, msg: str, details: dict):
        pass

    @property
    def c_callback(self) -> MaaEventCallback:
        return self._c_sink_agent

    @property
    def c_callback_arg(self) -> ctypes.c_void_p:
        return ctypes.c_void_p.from_buffer(ctypes.py_object(self))

    @staticmethod
    def _gen_c_param(
        sink: "EventSink",
    ) -> Tuple[MaaEventCallback, ctypes.c_void_p]:
        return sink.c_callback, sink.c_callback_arg

    @staticmethod
    def _notification_type(message: str) -> NotificationType:
        if message.endswith(".Starting"):
            return NotificationType.Starting
        elif message.endswith(".Succeeded"):
            return NotificationType.Succeeded
        elif message.endswith(".Failed"):
            return NotificationType.Failed
        else:
            return NotificationType.Unknown

    @staticmethod
    @MaaEventCallback
    def _c_sink_agent(
        handle: ctypes.c_void_p,
        msg: ctypes.c_char_p,
        details_json: ctypes.c_char_p,
        callback_arg: ctypes.c_void_p,
    ):
        if not callback_arg:
            return

        self: EventSink = ctypes.cast(callback_arg, ctypes.py_object).value

        self._on_raw_notification(
            handle, msg.decode(), json.loads(details_json.decode())
        )
