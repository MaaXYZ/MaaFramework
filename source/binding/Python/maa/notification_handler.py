import ctypes
import json
from abc import ABC
from typing import Optional, Tuple
from enum import Enum
from dataclasses import dataclass

from .define import MaaNotificationCallback


# class NotificationEvent(Enum):
#     ResourceLoading = 1
#     ControllerAction = 2
#     TaskerTask = 3
#     TaskNextList = 4
#     TaskRecognition = 5
#     TaskAction = 6


class NotificationType(Enum):
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
        self, type: NotificationType, detail: ResourceLoadingDetail
    ):
        pass

    @dataclass
    class ControllerActionDetail:
        ctrl_id: int
        uuid: str
        action: str

    def on_controller_action(
        self, type: NotificationType, detail: ControllerActionDetail
    ):
        pass

    @dataclass
    class TaskerTaskDetail:
        task_id: int
        entry: str
        uuid: str
        hash: str

    def on_tasker_task(self, type: NotificationType, detail: TaskerTaskDetail):
        pass

    @dataclass
    class TaskNextListDetail:
        task_id: int
        name: str
        next_list: list[str]

    def on_task_next_list(self, type: NotificationType, detail: TaskNextListDetail):
        pass

    @dataclass
    class TaskRecognitionDetail:
        task_id: int
        reco_id: int
        name: str

    def on_task_recognition(
        self, type: NotificationType, detail: TaskRecognitionDetail
    ):
        pass

    @dataclass
    class TaskActionDetail:
        task_id: int
        node_id: int
        name: str

    def on_task_action(self, type: NotificationType, detail: TaskActionDetail):
        pass

    def on_unknown_notification(self, msg: str, details: dict):
        pass

    def on_raw_notification(self, msg: str, details: dict):

        type = NotificationHandler._notification_type(msg)

        if msg.startswith("Resource.Loading"):
            detail = self.ResourceLoadingDetail(
                res_id=details["res_id"],
                hash=details["hash"],
                path=details["path"],
            )
            self.on_resource_loading(type, detail)

        elif msg.startswith("Controller.Action"):
            detail = self.ControllerActionDetail(
                ctrl_id=details["ctrl_id"],
                uuid=details["uuid"],
                action=details["action"],
            )
            self.on_controller_action(type, detail)

        elif msg.startswith("Tasker.Task"):
            detail = self.TaskerTaskDetail(
                task_id=details["task_id"],
                entry=details["entry"],
                uuid=details["uuid"],
                hash=details["hash"],
            )
            self.on_tasker_task(type, detail)

        elif msg.startswith("Task.NextList"):
            detail = self.TaskNextListDetail(
                task_id=details["task_id"],
                name=details["name"],
                next_list=details["list"],
            )
            self.on_task_next_list(type, detail)

        elif msg.startswith("Task.Recognition"):
            detail = self.TaskRecognitionDetail(
                task_id=details["task_id"],
                reco_id=details["reco_id"],
                name=details["name"],
            )
            self.on_task_recognition(type, detail)

        elif msg.startswith("Task.Action"):
            detail = self.TaskActionDetail(
                task_id=details["task_id"],
                node_id=details["node_id"],
                name=details["name"],
            )
            self.on_task_action(type, detail)

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
