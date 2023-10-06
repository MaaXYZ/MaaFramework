import ctypes


MaaApiCallback = ctypes.CFUNCTYPE(
    None, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_void_p
)
MaaBool = ctypes.c_uint8
MaaId = ctypes.c_uint64
MaaStatus = ctypes.c_int32

