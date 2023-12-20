import ctypes


MaaApiCallback = ctypes.CFUNCTYPE(
    None, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_void_p
)
MaaBool = ctypes.c_uint8
MaaId = ctypes.c_uint64
MaaStatus = ctypes.c_int32


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
