import ctypes
import numpy
import json

from typing import Any, Dict, Optional, Tuple

from .library import Library
from .define import MaaBool
from .buffer import *


class SyncContext:
    _handle: MaaSyncContextHandle

    def __init__(self, handle: MaaSyncContextHandle):
        self._set_api_properties()

        self._handle = handle
        if not self._handle:
            raise ValueError("handle is None")

    def run_task(self, task_name: str, task_param: str) -> bool:
        """
        Sync context run task.

        :param task_name: task name
        :param task_param: task param

        :return: True if success, otherwise False
        """
        return bool(
            Library.framework.MaaSyncContextRunTask(
                self._handle, task_name.encode("utf-8"), task_param.encode("utf-8")
            )
        )

    def run_recognizer(
        self,
        image: numpy.ndarray,
        task_name: str,
        task_param: Dict,
    ) -> Optional[Tuple[Tuple[int, int, int, int], str]]:
        """
        Sync context run recognizer.

        :param image: image
        :param task_name: task name
        :param task_param: task param

        :return: (x, y, width, height), detail
        """

        image_buffer = ImageBuffer()
        image_buffer.set(image)

        rect_buffer = RectBuffer()
        detail_buffer = StringBuffer()

        ret = Library.framework.MaaSyncContextRunRecognizer(
            self._handle,
            image_buffer.c_handle(),
            task_name.encode("utf-8"),
            json.dumps(task_param).encode("utf-8"),
            rect_buffer.c_handle(),
            detail_buffer.c_handle(),
        )

        if not ret:
            return None

        return (
            rect_buffer.get(),
            detail_buffer.get(),
        )

    def run_action(
        self,
        task_name: str,
        task_param: Dict,
        cur_box: Tuple[int, int, int, int],
        cur_rec_detail: str,
    ) -> Optional[str]:
        """
        Sync context run action.

        :param task_name: task name
        :param task_param: task param
        :param cur_box: current box
        :param cur_rec_detail: current recognizer detail

        :return: detail
        """

        rect_buffer = RectBuffer()
        rect_buffer.set(cur_box)

        return bool(
            Library.framework.MaaSyncContextRunAction(
                self._handle,
                task_name.encode("utf-8"),
                json.dumps(task_param).encode("utf-8"),
                rect_buffer.c_handle(),
                cur_rec_detail.encode("utf-8"),
            )
        )

    def click(self, x: int, y: int) -> bool:
        """
        Sync context click.

        :param x: x
        :param y: y

        :return: True if success, otherwise False
        """
        return bool(Library.framework.MaaSyncContextClick(self._handle, x, y))

    def swipe(self, x1: int, y1: int, x2: int, y2: int, duration: int) -> bool:
        """
        Sync context swipe.

        :param x1: x1
        :param y1: y1
        :param x2: x2
        :param y2: y2
        :param duration: duration

        :return: True if success, otherwise False
        """
        return bool(
            Library.framework.MaaSyncContextSwipe(
                self._handle, x1, y1, x2, y2, duration
            )
        )

    def press_key(self, key: int) -> bool:
        """
        Sync context press key.

        :param key: key

        :return: True if success, otherwise False
        """
        return bool(Library.framework.MaaSyncContextPressKey(self._handle, key))

    def input_text(self, text: str) -> bool:
        """
        Sync context input text.

        :param text: text

        :return: True if success, otherwise False
        """
        return bool(
            Library.framework.MaaSyncContextInputText(
                self._handle, text.encode("utf-8")
            )
        )

    def touch_down(self, contact: int, x: int, y: int, pressure: int) -> bool:
        """
        Sync context touch down.

        :param contact: contact
        :param x: x
        :param y: y
        :param pressure: pressure

        :return: True if success, otherwise False
        """
        return bool(
            Library.framework.MaaSyncContextTouchDown(
                self._handle, contact, x, y, pressure
            )
        )

    def touch_move(self, contact: int, x: int, y: int, pressure: int) -> bool:
        """
        Sync context touch move.

        :param contact: contact
        :param x: x
        :param y: y
        :param pressure: pressure

        :return: True if success, otherwise False
        """
        return bool(
            Library.framework.MaaSyncContextTouchMove(
                self._handle, contact, x, y, pressure
            )
        )

    def touch_up(self, contact: int) -> bool:
        """
        Sync context touch up.

        :param contact: contact

        :return: True if success, otherwise False
        """
        return bool(Library.framework.MaaSyncContextTouchUp(self._handle, contact))

    def screencap(self) -> Optional[numpy.ndarray]:
        """
        Sync context screencap.

        :return: image
        """
        image_buffer = ImageBuffer()
        ret = Library.framework.MaaSyncContextScreencap(
            self._handle, image_buffer.c_handle()
        )
        if not ret:
            return None
        return image_buffer.get()

    def get_task_result(self, task_name: str) -> Optional[dict]:
        """
        Sync context get task result.

        :param task_name: task name

        :return: task result
        """
        string_buffer = StringBuffer()
        ret = Library.framework.MaaSyncContextGetTaskResult(
            self._handle, task_name.encode("utf-8"), string_buffer.c_handle()
        )
        if not ret:
            return None
        return json.loads(string_buffer.get())

    _api_properties_initialized: bool = False

    @staticmethod
    def _set_api_properties():
        if SyncContext._api_properties_initialized:
            return

        SyncContext._api_properties_initialized = True

        Library.framework.MaaSyncContextRunTask.restype = MaaBool
        Library.framework.MaaSyncContextRunTask.argtypes = [
            MaaSyncContextHandle,
            MaaStringView,
            MaaStringView,
        ]

        Library.framework.MaaSyncContextRunRecognizer.restype = MaaBool
        Library.framework.MaaSyncContextRunRecognizer.argtypes = [
            MaaSyncContextHandle,
            MaaStringBufferHandle,
            MaaStringView,
            MaaStringView,
            MaaRectHandle,
            MaaStringBufferHandle,
        ]

        Library.framework.MaaSyncContextRunAction.restype = MaaBool
        Library.framework.MaaSyncContextRunAction.argtypes = [
            MaaSyncContextHandle,
            MaaStringView,
            MaaStringView,
            MaaRectHandle,
            MaaStringBufferHandle,
        ]

        Library.framework.MaaSyncContextClick.restype = MaaBool
        Library.framework.MaaSyncContextClick.argtypes = [
            MaaSyncContextHandle,
            ctypes.c_int32,
            ctypes.c_int32,
        ]

        Library.framework.MaaSyncContextSwipe.restype = MaaBool
        Library.framework.MaaSyncContextSwipe.argtypes = [
            MaaSyncContextHandle,
            ctypes.c_int32,
            ctypes.c_int32,
            ctypes.c_int32,
            ctypes.c_int32,
            ctypes.c_int32,
        ]

        Library.framework.MaaSyncContextPressKey.restype = MaaBool
        Library.framework.MaaSyncContextPressKey.argtypes = [
            MaaSyncContextHandle,
            ctypes.c_int32,
        ]

        Library.framework.MaaSyncContextInputText.restype = MaaBool
        Library.framework.MaaSyncContextInputText.argtypes = [
            MaaSyncContextHandle,
            MaaStringView,
        ]

        Library.framework.MaaSyncContextTouchDown.restype = MaaBool
        Library.framework.MaaSyncContextTouchDown.argtypes = [
            MaaSyncContextHandle,
            ctypes.c_int32,
            ctypes.c_int32,
            ctypes.c_int32,
            ctypes.c_int32,
        ]

        Library.framework.MaaSyncContextTouchMove.restype = MaaBool
        Library.framework.MaaSyncContextTouchMove.argtypes = [
            MaaSyncContextHandle,
            ctypes.c_int32,
            ctypes.c_int32,
            ctypes.c_int32,
            ctypes.c_int32,
        ]

        Library.framework.MaaSyncContextTouchUp.restype = MaaBool
        Library.framework.MaaSyncContextTouchUp.argtypes = [
            MaaSyncContextHandle,
            ctypes.c_int32,
        ]

        Library.framework.MaaSyncContextScreencap.restype = MaaBool
        Library.framework.MaaSyncContextScreencap.argtypes = [
            MaaSyncContextHandle,
            MaaImageBufferHandle,
        ]

        Library.framework.MaaSyncContextGetTaskResult.restype = MaaBool
        Library.framework.MaaSyncContextGetTaskResult.argtypes = [
            MaaSyncContextHandle,
            MaaStringView,
            MaaStringBufferHandle,
        ]
