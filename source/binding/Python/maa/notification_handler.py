import ctypes
import json
from abc import ABC
from typing import Optional, Tuple, Any
from enum import IntEnum
from dataclasses import dataclass

from .define import MaaNotificationCallback


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


class NotificationHandler(ABC):

    @dataclass
    class ResourceLoadingDetail:
        res_id: int
        hash: str
        path: str

    def on_resource_loading(
        self, noti_type: NotificationType, detail: ResourceLoadingDetail
    ):
        pass

    @dataclass
    class ControllerActionDetail:
        ctrl_id: int
        uuid: str
        action: str

    def on_controller_action(
        self, noti_type: NotificationType, detail: ControllerActionDetail
    ):
        pass

    @dataclass
    class TaskerTaskDetail:
        task_id: int
        entry: str
        uuid: str
        hash: str

    def on_tasker_task(self, noti_type: NotificationType, detail: TaskerTaskDetail):
        pass

    @dataclass
    class NodeNextListDetail:
        task_id: int
        name: str
        next_list: list[str]
        focus: Any

    def on_node_next_list(
        self, noti_type: NotificationType, detail: NodeNextListDetail
    ):
        pass

    @dataclass
    class NodeRecognitionDetail:
        task_id: int
        reco_id: int
        name: str
        focus: Any

    def on_node_recognition(
        self, noti_type: NotificationType, detail: NodeRecognitionDetail
    ):
        pass

    @dataclass
    class NodeActionDetail:
        task_id: int
        node_id: int
        name: str
        focus: Any

    def on_node_action(self, noti_type: NotificationType, detail: NodeActionDetail):
        pass

    def on_unknown_notification(self, msg: str, details: dict):
        pass

    def on_raw_notification(self, msg: str, details: dict):

        noti_type = NotificationHandler._notification_type(msg)

        if msg.startswith("Resource.Loading"):
            detail = self.ResourceLoadingDetail(
                res_id=details["res_id"],
                hash=details["hash"],
                path=details["path"],
            )
            self.on_resource_loading(noti_type, detail)

        elif msg.startswith("Controller.Action"):
            detail = self.ControllerActionDetail(
                ctrl_id=details["ctrl_id"],
                uuid=details["uuid"],
                action=details["action"],
            )
            self.on_controller_action(noti_type, detail)

        elif msg.startswith("Tasker.Task"):
            detail = self.TaskerTaskDetail(
                task_id=details["task_id"],
                entry=details["entry"],
                uuid=details["uuid"],
                hash=details["hash"],
            )
            self.on_tasker_task(noti_type, detail)

        elif msg.startswith("Node.NextList"):
            detail = self.NodeNextListDetail(
                task_id=details["task_id"],
                name=details["name"],
                next_list=details["list"],
                focus=details["focus"],
            )
            self.on_node_next_list(noti_type, detail)

        elif msg.startswith("Node.Recognition"):
            detail = self.NodeRecognitionDetail(
                task_id=details["task_id"],
                reco_id=details["reco_id"],
                name=details["name"],
                focus=details["focus"],
            )
            self.on_node_recognition(noti_type, detail)

        elif msg.startswith("Node.Action"):
            detail = self.NodeActionDetail(
                task_id=details["task_id"],
                node_id=details["node_id"],
                name=details["name"],
                focus=details["focus"],
            )
            self.on_node_action(noti_type, detail)

        else:
            self.on_unknown_notification(msg, details)

    @property
    def c_callback(self) -> MaaNotificationCallback:
        return self._c_notification_agent

    @property
    def c_callback_arg(self) -> ctypes.c_void_p:
        return ctypes.c_void_p.from_buffer(ctypes.py_object(self))

    @staticmethod
    def _gen_c_param(
        handler: Optional["NotificationHandler"],
    ) -> Tuple[MaaNotificationCallback, ctypes.c_void_p]:
        if handler:
            return handler.c_callback, handler.c_callback_arg
        else:
            return NotificationHandler._c_notification_agent, ctypes.c_void_p()

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
    @MaaNotificationCallback
    def _c_notification_agent(
        msg: ctypes.c_char_p,
        details_json: ctypes.c_char_p,
        callback_arg: ctypes.c_void_p,
    ):
        if not callback_arg:
            return

        self: NotificationHandler = ctypes.cast(callback_arg, ctypes.py_object).value

        self.on_raw_notification(msg.decode(), json.loads(details_json.decode()))
