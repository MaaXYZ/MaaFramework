import ctypes
from enum import Enum


MaaApiCallback = ctypes.CFUNCTYPE(
    None, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_void_p
)
MaaBool = ctypes.c_uint8
MaaId = ctypes.c_uint64


class MaaStatusEnum(Enum):
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


# TODO: tidy

MaaOptionValueSize = ctypes.c_uint64
MaaOptionValue = ctypes.c_void_p

MaaOption = ctypes.c_int32
MaaCtrlOption = MaaOption


class MaaCtrlOptionEnum:
    Invalid: MaaCtrlOption = 0

    # Only one of long and short side can be set, and the other is automatically scaled according to the aspect ratio.
    # value: int, eg: 1920; val_size: sizeof(int)
    ScreenshotTargetLongSide: MaaCtrlOption = 1

    # Only one of long and short side can be set, and the other is automatically scaled according to the aspect ratio.
    # value: int, eg: 1080; val_size: sizeof(int)
    ScreenshotTargetShortSide: MaaCtrlOption = 2

    # For StartApp
    # value: string, eg: "com.hypergryph.arknights/com.u8.sdk.U8UnityContext"; val_size: string length
    DefaultAppPackageEntry: MaaCtrlOption = 3

    # For StopApp
    # value: string, eg: "com.hypergryph.arknights"; val_size: string length
    DefaultAppPackage: MaaCtrlOption = 4

    # Dump all screenshots and actions
    # this option will || with MaaGlobalOptionEnum.Recording
    # value: bool, eg: true; val_size: sizeof(bool)
    Recording: MaaCtrlOption = 5


MaaControllerHandle = ctypes.c_void_p

MaaStringView = ctypes.c_char_p
MaaStringBufferHandle = ctypes.c_void_p

MaaSize = ctypes.c_size_t

MaaImageBufferHandle = ctypes.c_void_p
MaaImageRawData = ctypes.c_void_p

MaaRectHandle = ctypes.c_void_p

MaaSyncContextHandle = ctypes.c_void_p

MaaControllerHandle = ctypes.c_void_p

MaaStatus = ctypes.c_int32
MaaCtrlId = MaaId
MaaAdbControllerType = ctypes.c_int32


class MaaAdbControllerTypeEnum:
    Invalid: MaaAdbControllerType = 0

    Touch_Adb: MaaAdbControllerType = 1
    Touch_MiniTouch: MaaAdbControllerType = 2
    Touch_MaaTouch: MaaAdbControllerType = 3
    Touch_Mask: MaaAdbControllerType = 0xFF

    Key_Adb: MaaAdbControllerType = 1 << 8
    Key_MaaTouch: MaaAdbControllerType = 2 << 8
    Key_Mask: MaaAdbControllerType = 0xFF00

    Input_Preset_Adb: MaaAdbControllerType = Touch_Adb | Key_Adb
    Input_Preset_Minitouch: MaaAdbControllerType = Touch_MiniTouch | Key_Adb
    Input_Preset_Maatouch: MaaAdbControllerType = Touch_MaaTouch | Key_MaaTouch

    Screencap_FastestWay: MaaAdbControllerType = 1 << 16
    Screencap_RawByNetcat: MaaAdbControllerType = 2 << 16
    Screencap_RawWithGzip: MaaAdbControllerType = 3 << 16
    Screencap_Encode: MaaAdbControllerType = 4 << 16
    Screencap_EncodeToFile: MaaAdbControllerType = 5 << 16
    Screencap_MinicapDirect: MaaAdbControllerType = 6 << 16
    Screencap_MinicapStream: MaaAdbControllerType = 7 << 16
    Screencap_Mask: MaaAdbControllerType = 0xFF0000


MaaControllerCallback = MaaApiCallback
MaaTransparentArg = ctypes.c_void_p
MaaCallbackTransparentArg = MaaTransparentArg

MaaInstanceHandle = ctypes.c_void_p
MaaInstanceCallback = MaaApiCallback
MaaResourceHandle = ctypes.c_void_p

MaaTaskId = MaaId
MaaCustomRecognizerHandle = ctypes.c_void_p
MaaCustomActionHandle = ctypes.c_void_p

MaaResourceCallback = MaaApiCallback
MaaResId = MaaId
