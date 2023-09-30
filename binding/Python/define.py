import ctypes
from enum import Enum


maa_api_callback = ctypes.CFUNCTYPE(
    None, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_void_p)
maa_bool = ctypes.c_uint8
maa_id = ctypes.c_uint64


class maa_status(Enum):
    invalid = 0
    pending = 1000
    running = 2000
    success = 3000
    failure = 4000
