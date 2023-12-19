import ctypes
import json
from typing import Callable, Any, Optional, Dict

from .define import MaaApiCallback


Callback = Callable[[str, Any, Any], None]


class CallbackAgent:
    def __init__(self, callback: Optional[Callback] = None, callback_arg: Any = None):
        self._callback = callback
        self._callback_arg = callback_arg

    def c_callback(self) -> MaaApiCallback:
        return self._c_callback_agent

    def c_callback_arg(self) -> ctypes.c_void_p:
        return ctypes.c_void_p.from_buffer(ctypes.py_object(self))

    @MaaApiCallback
    def _c_callback_agent(
        msg: ctypes.c_char_p,
        details_json: ctypes.c_char_p,
        callback_arg: ctypes.c_void_p,
    ):
        if not callback_arg:
            return

        self: CallbackAgent = ctypes.cast(callback_arg, ctypes.py_object).value
        if not self._callback:
            return

        self._callback(
            msg.decode("utf-8"),
            json.loads(details_json.decode("utf-8")),
            self._callback_arg,
        )
