import ctypes
from enum import Enum


MaaApiCallback = ctypes.CFUNCTYPE(
    None, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_void_p
)
MaaBool = ctypes.c_uint8
MaaId = ctypes.c_uint64


class MaaStatus(Enum):
    invalid = 0
    pending = 1000
    running = 2000
    success = 3000
    failure = 4000


class MaaCustomRecognizer(ctypes.Structure):
    AnalyzeFunc = ctypes.CFUNCTYPE(
        MaaBool,
        ctypes.c_void_p,
        ctypes.c_void_p,
        ctypes.c_char_p,
        ctypes.c_char_p,
        ctypes.c_void_p,
        ctypes.c_void_p,
        ctypes.c_void_p,
    )
    _fields_ = [
        ("analyze", AnalyzeFunc),
    ]


class MaaCustomAction(ctypes.Structure):
    RunFunc = ctypes.CFUNCTYPE(
        MaaBool,
        ctypes.c_void_p,
        ctypes.c_char_p,
        ctypes.c_char_p,
        ctypes.c_void_p,
        ctypes.c_char_p,
        ctypes.c_void_p,
    )
    StopFunc = ctypes.CFUNCTYPE(
        None,
        ctypes.c_void_p,
    )
    _fields_ = [
        ("action", RunFunc),
        ("stop", StopFunc),
    ]
