import ctypes
from abc import ABC, abstractmethod
from typing import Optional

from .buffer import ImageBuffer, StringBuffer
from .define import *


class CustomControllerAgent(ABC):
    _handle: MaaCustomControllerCallbacks

    def __init__(self):
        self._handle = MaaCustomControllerCallbacks(
            self._c_connect_agent,
            self._c_request_uuid_agent,
            self._c_start_app_agent,
            self._c_stop_app_agent,
            self._c_screencap_agent,
            self._c_click_agent,
            self._c_swipe_agent,
            self._c_touch_down_agent,
            self._c_touch_move_agent,
            self._c_touch_up_agent,
            self._c_press_key_agent,
            self._c_input_text_agent,
        )

    @property
    def c_handle(self) -> MaaCustomControllerCallbacks:
        return MaaCustomControllerCallbacks(self._handle)

    @property
    def c_arg(self) -> ctypes.c_void_p:
        return ctypes.c_void_p.from_buffer(ctypes.py_object(self))

    @abstractmethod
    def connect(self) -> bool:
        raise NotImplementedError

    @abstractmethod
    def request_uuid(self) -> Optional[str]:
        raise NotImplementedError

    @MaaCustomControllerCallbacks.ConnectFunc
    def _c_connect_agent(
        trans_arg: ctypes.c_void_p,
    ) -> MaaBool:
        if not trans_arg:
            return False

        self: CustomControllerAgent = ctypes.cast(
            trans_arg,
            ctypes.py_object,
        ).value

        return self.connect()

    @MaaCustomControllerCallbacks.RequestUuidFunc
    def _c_request_uuid_agent(
        trans_arg: ctypes.c_void_p,
        c_buffer: MaaStringBufferHandle,
    ) -> MaaBool:
        if not trans_arg:
            return False

        self: CustomControllerAgent = ctypes.cast(
            trans_arg,
            ctypes.py_object,
        ).value

        uuid_buffer = StringBuffer(c_buffer)
        uuid = self.request_uuid()

        if not uuid:
            return False

        uuid_buffer.write(uuid)
        return True

    @abstractmethod
    def start_app(self, intent: str) -> bool:
        raise NotImplementedError

    @MaaCustomControllerCallbacks.StartAppFunc
    def _c_start_app_agent(
        c_intent: ctypes.c_char_p,
        trans_arg: ctypes.c_void_p,
    ) -> MaaBool:
        if not trans_arg:
            return False

        self: CustomControllerAgent = ctypes.cast(
            trans_arg,
            ctypes.py_object,
        ).value

        return self.start_app(c_intent.value)

    @abstractmethod
    def stop_app(self, intent: str) -> bool:
        raise NotImplementedError

    @MaaCustomControllerCallbacks.StartAppFunc
    def _c_stop_app_agent(
        c_intent: ctypes.c_char_p,
        trans_arg: ctypes.c_void_p,
    ) -> MaaBool:
        if not trans_arg:
            return False

        self: CustomControllerAgent = ctypes.cast(
            trans_arg,
            ctypes.py_object,
        ).value

        return self.stop_app(c_intent.value)

    @abstractmethod
    def screencap(self) -> Optional[numpy.ndarray]:
        raise NotImplementedError

    @MaaCustomControllerCallbacks.ScreencapFunc
    def _c_screencap_agent(
        trans_arg: ctypes.c_void_p,
        c_buffer: MaaStringBufferHandle,
    ) -> MaaBool:
        if not trans_arg:
            return False

        self: CustomControllerAgent = ctypes.cast(
            trans_arg,
            ctypes.py_object,
        ).value

        ret = self.screencap()

        if not ret:
            return False

        buffer = ImageBuffer()
        buffer.set(ret)

        return True

    @abstractmethod
    def click(self, x: int, y: int) -> bool:
        raise NotImplementedError

    @MaaCustomControllerCallbacks.ClickFunc
    def _c_click_agent(
        c_x: ctypes.c_int32,
        c_y: ctypes.c_int32,
        trans_arg: ctypes.c_void_p,
    ) -> MaaBool:
        if not trans_arg:
            return False

        self: CustomControllerAgent = ctypes.cast(
            trans_arg,
            ctypes.py_object,
        ).value

        return self.click(c_x, c_y)

    @abstractmethod
    def swipe(self, x1: int, y1: int, x2: int, y2: int) -> bool:
        raise NotImplementedError

    @MaaCustomControllerCallbacks.SwipeFunc
    def _c_swipe_agent(
        c_x1: ctypes.c_int32,
        c_y1: ctypes.c_int32,
        c_x2: ctypes.c_int32,
        c_y2: ctypes.c_int32,
        trans_arg: ctypes.c_void_p,
    ) -> MaaBool:
        if not trans_arg:
            return False

        self: CustomControllerAgent = ctypes.cast(
            trans_arg,
            ctypes.py_object,
        ).value

        return self.swipe(c_x1, c_y1, c_x2, c_y2)

    @abstractmethod
    def touch_down(
        self,
        contact: int,
        x: int,
        y: int,
        pressure: int,
    ) -> bool:
        raise NotImplementedError

    @MaaCustomControllerCallbacks.TouchDownFunc
    def _c_touch_down_agent(
        c_contact: ctypes.c_int32,
        c_x: ctypes.c_int32,
        c_y: ctypes.c_int32,
        c_pressure: ctypes.c_int32,
        trans_arg: ctypes.c_void_p,
    ) -> MaaBool:
        if not trans_arg:
            return False

        self: CustomControllerAgent = ctypes.cast(
            trans_arg,
            ctypes.py_object,
        ).value

        return self.touch_down(c_contact, c_x, c_y, c_pressure)

    @abstractmethod
    def touch_move(
        self,
        contact: int,
        x: int,
        y: int,
        pressure: int,
    ) -> bool:
        raise NotImplementedError

    @MaaCustomControllerCallbacks.TouchMoveFunc
    def _c_touch_move_agent(
        c_contact: ctypes.c_int32,
        c_x: ctypes.c_int32,
        c_y: ctypes.c_int32,
        c_pressure: ctypes.c_int32,
        trans_arg: ctypes.c_void_p,
    ) -> MaaBool:
        if not trans_arg:
            return False

        self: CustomControllerAgent = ctypes.cast(
            trans_arg,
            ctypes.py_object,
        ).value

        return self.touch_move(c_contact, c_x, c_y, c_pressure)

    @abstractmethod
    def touch_up(self, contact: int) -> bool:
        raise NotImplementedError

    @MaaCustomControllerCallbacks.TouchUpFunc
    def _c_touch_up_agent(
        c_contact: ctypes.c_int32,
        trans_arg: ctypes.c_void_p,
    ) -> MaaBool:
        if not trans_arg:
            return False

        self: CustomControllerAgent = ctypes.cast(
            trans_arg,
            ctypes.py_object,
        ).value

        return self.touch_up(c_contact)

    @abstractmethod
    def press_key(self, keycode: int) -> bool:
        raise NotImplementedError

    @MaaCustomControllerCallbacks.PressKeyFunc
    def _c_press_key_agent(
        c_keycode: ctypes.c_int32,
        trans_arg: ctypes.c_void_p,
    ) -> MaaBool:
        if not trans_arg:
            return False

        self: CustomControllerAgent = ctypes.cast(
            trans_arg,
            ctypes.py_object,
        ).value

        return self.press_key(c_keycode)

    @abstractmethod
    def input_text(self, text: str) -> bool:
        raise NotImplementedError

    @MaaCustomControllerCallbacks.InputTextFunc
    def _c_input_text_agent(
        c_text: ctypes.c_char_p,
        trans_arg: ctypes.c_void_p,
    ) -> MaaBool:
        if not trans_arg:
            return False

        self: CustomControllerAgent = ctypes.cast(
            trans_arg,
            ctypes.py_object,
        ).value

        return self.input_text(c_text.value)
