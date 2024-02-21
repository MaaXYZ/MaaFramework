# from __future__ import annotations
import ctypes
from dataclasses import dataclass
from enum import Enum
from typing import List, Tuple, Union

import numpy


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
    Touch_AutoDetect: MaaAdbControllerType = 0xFF - 1

    Key_Adb: MaaAdbControllerType = 1 << 8
    Key_MaaTouch: MaaAdbControllerType = 2 << 8
    Key_AutoDetect: MaaAdbControllerType = 0xFF00 - (1 << 8)

    Input_Preset_Adb: MaaAdbControllerType = Touch_Adb | Key_Adb
    Input_Preset_Minitouch: MaaAdbControllerType = Touch_MiniTouch | Key_Adb
    Input_Preset_Maatouch: MaaAdbControllerType = Touch_MaaTouch | Key_MaaTouch
    Input_Preset_AutoDetect: MaaAdbControllerType = Touch_AutoDetect | Key_AutoDetect

    Screencap_RawByNetcat: MaaAdbControllerType = 2 << 16
    Screencap_RawWithGzip: MaaAdbControllerType = 3 << 16
    Screencap_Encode: MaaAdbControllerType = 4 << 16
    Screencap_EncodeToFile: MaaAdbControllerType = 5 << 16
    Screencap_MinicapDirect: MaaAdbControllerType = 6 << 16
    Screencap_MinicapStream: MaaAdbControllerType = 7 << 16
    Screencap_FastestLosslessWay: MaaAdbControllerType = 0xFF0000 - (2 << 16)
    Screencap_FastestWay: MaaAdbControllerType = 0xFF0000 - (1 << 16)


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


@dataclass
class Rect:
    x: int = 0
    y: int = 0
    w: int = 0
    h: int = 0

    def __add__(
        self,
        other: Union[
            "Rect",
            Tuple[int, int, int, int],
            List[int],
        ],
    ):
        if (
            isinstance(other, Rect)
            or isinstance(other, tuple)
            or (isinstance(other, list) and len(other) == 4)
        ):
            x1, y1, w1, h1 = self
            x2, y2, w2, h2 = other
            return Rect(
                x1 + x2,
                y1 + y2,
                w1 + w2,
                h1 + h2,
            )

        raise TypeError(f"Cannot add {type(other).__name__} to Rect")

    def __iter__(self):
        yield self.x
        yield self.y
        yield self.w
        yield self.h

    def __getitem__(self, key):
        return self.roi[key]

    @property
    def roi(self):
        return list(self)


RectType = Union[
    Rect,
    List[int],
    numpy.ndarray,
    Tuple[int, int, int, int],
]

MaaDbgControllerType = ctypes.c_int32


class MaaDbgControllerTypeEnum:
    Invalid: MaaAdbControllerType = 0
    CarouselImage: MaaAdbControllerType = 1
    ReplayRecording: MaaAdbControllerType = 2


MaaWin32ControllerType = ctypes.c_int32


class MaaWin32ControllerTypeEnum:
    Invalid = 0

    Touch_SendMessage: MaaWin32ControllerType = 1

    Key_SendMessage: MaaWin32ControllerType = 1 << 8

    Screencap_GDI: MaaWin32ControllerType = 1 << 16
    Screencap_DXGI_DesktopDup: MaaWin32ControllerType = 2 << 16
    # Screencap_DXGI_BackBuffer = 3 << 16
    Screencap_DXGI_FramePool: MaaWin32ControllerType = 4 << 16


MaaWin32Hwnd = ctypes.c_void_p


class MaaCustomRecognizer(ctypes.Structure):
    AnalyzeFunc = ctypes.CFUNCTYPE(
        MaaBool,
        MaaSyncContextHandle,
        MaaImageBufferHandle,
        MaaStringView,
        MaaStringView,
        MaaTransparentArg,
        MaaRectHandle,
        MaaStringBufferHandle,
    )
    _fields_ = [
        ("analyze", AnalyzeFunc),
    ]


class MaaCustomAction(ctypes.Structure):
    RunFunc = ctypes.CFUNCTYPE(
        MaaBool,
        MaaSyncContextHandle,
        MaaStringView,
        MaaStringView,
        MaaRectHandle,
        MaaStringView,
        MaaTransparentArg,
    )
    StopFunc = ctypes.CFUNCTYPE(
        None,
        MaaTransparentArg,
    )
    _fields_ = [
        ("action", RunFunc),
        ("stop", StopFunc),
    ]


c_int32_p = ctypes.POINTER(ctypes.c_int32)


class MaaCustomControllerAPI(ctypes.Structure):
    ConnectFunc = ctypes.CFUNCTYPE(
        MaaBool,
        MaaTransparentArg,
    )
    RequestUuidFunc = ctypes.CFUNCTYPE(
        MaaBool,
        MaaTransparentArg,
        MaaStringBufferHandle,
    )
    RequestResolutionFunc = ctypes.CFUNCTYPE(
        MaaBool,
        MaaTransparentArg,
        c_int32_p,
        c_int32_p,
    )
    StartAppFunc = ctypes.CFUNCTYPE(
        MaaBool,
        MaaStringView,
        MaaTransparentArg,
    )
    StopAppFunc = ctypes.CFUNCTYPE(
        MaaBool,
        MaaStringView,
        MaaTransparentArg,
    )
    ScreencapFunc = ctypes.CFUNCTYPE(
        MaaBool,
        MaaTransparentArg,
        MaaImageBufferHandle,
    )
    ClickFunc = ctypes.CFUNCTYPE(
        MaaBool,
        ctypes.c_int32,
        ctypes.c_int32,
        MaaTransparentArg,
    )
    SwipeFunc = ctypes.CFUNCTYPE(
        MaaBool,
        ctypes.c_int32,
        ctypes.c_int32,
        ctypes.c_int32,
        ctypes.c_int32,
        ctypes.c_int32,
        MaaTransparentArg,
    )
    TouchDownFunc = ctypes.CFUNCTYPE(
        MaaBool,
        ctypes.c_int32,
        ctypes.c_int32,
        ctypes.c_int32,
        ctypes.c_int32,
        MaaTransparentArg,
    )
    TouchMoveFunc = ctypes.CFUNCTYPE(
        MaaBool,
        ctypes.c_int32,
        ctypes.c_int32,
        ctypes.c_int32,
        ctypes.c_int32,
        MaaTransparentArg,
    )
    TouchUpFunc = ctypes.CFUNCTYPE(
        MaaBool,
        ctypes.c_int32,
        MaaTransparentArg,
    )
    PressKeyFunc = ctypes.CFUNCTYPE(
        MaaBool,
        ctypes.c_int32,
        MaaTransparentArg,
    )
    InputTextFunc = ctypes.CFUNCTYPE(
        MaaBool,
        MaaStringView,
        MaaTransparentArg,
    )
    _fields_ = [
        ("connect", ConnectFunc),
        ("request_uuid", RequestUuidFunc),
        ("request_resolution", RequestResolutionFunc),
        ("start_app", StartAppFunc),
        ("stop_app", StopAppFunc),
        ("screencap", ScreencapFunc),
        ("click", ClickFunc),
        ("swipe", SwipeFunc),
        ("touch_down", TouchDownFunc),
        ("touch_move", TouchMoveFunc),
        ("touch_up", TouchUpFunc),
        ("press_key", PressKeyFunc),
        ("input_text", InputTextFunc),
    ]


MaaCustomControllerHandle = ctypes.POINTER(MaaCustomControllerAPI)


MaaThriftControllerType = ctypes.c_int32


class MaaThriftControllerTypeEnum:
    Invalid: MaaThriftControllerType = 0
    Socket: MaaThriftControllerType = 1
    UnixDomainSocket: MaaThriftControllerType = 2
