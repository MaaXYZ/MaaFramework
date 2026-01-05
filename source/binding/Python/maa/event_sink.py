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
    """通知类型枚举 / Notification type enumeration

    用于标识事件回调的状态类型。
    Used to identify the status type of event callbacks.

    Attributes:
        Unknown: 未知类型 / Unknown type
        Starting: 开始 / Starting
        Succeeded: 成功 / Succeeded
        Failed: 失败 / Failed
    """

    Unknown = 0
    Starting = 1
    Succeeded = 2
    Failed = 3


class EventSink(ABC):
    """事件监听器基类 / Event sink base class

    用于接收 MaaFramework 各种事件回调的抽象基类。
    派生类包括 ResourceEventSink、ControllerEventSink、TaskerEventSink、ContextEventSink。
    Abstract base class for receiving various event callbacks from MaaFramework.
    Derived classes include ResourceEventSink, ControllerEventSink, TaskerEventSink, ContextEventSink.
    """

    def on_unknown_notification(self, instance, msg: str, details: dict):
        """处理未知类型的通知 / Handle unknown notification

        当收到无法识别的通知时调用。
        Called when an unrecognized notification is received.

        Args:
            instance: 相关实例对象 / Related instance object
            msg: 消息类型 / Message type
            details: 消息详情 / Message details
        """
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
