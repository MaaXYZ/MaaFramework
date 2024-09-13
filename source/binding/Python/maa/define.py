# from __future__ import annotations
import ctypes
from dataclasses import dataclass
from enum import Enum
from typing import List, Tuple, Union, Dict, Optional

import numpy

MaaBool = ctypes.c_uint8
MaaSize = ctypes.c_size_t
MaaNullSize = MaaSize(-1)

MaaId = ctypes.c_int64
MaaCtrlId = MaaId
MaaResId = MaaId
MaaTaskId = MaaId
MaaRecoId = MaaId
MaaNodeId = MaaId
MaaInvalidId = MaaId(0)

MaaStringBufferHandle = ctypes.c_void_p
MaaImageBufferHandle = ctypes.c_void_p
MaaRectHandle = ctypes.c_void_p
MaaStringListBufferHandle = ctypes.c_void_p
MaaImageListBufferHandle = ctypes.c_void_p

MaaResourceHandle = ctypes.c_void_p
MaaControllerHandle = ctypes.c_void_p
MaaTaskerHandle = ctypes.c_void_p
MaaContextHandle = ctypes.c_void_p

MaaStatus = ctypes.c_int32


class MaaStatusEnum(Enum):
    invalid = 0
    pending = 1000
    running = 2000
    success = 3000
    failure = 4000


MaaLoggingLevel = ctypes.c_int32


class MaaLoggingLevelEunm:
    Off: MaaLoggingLevel = 0
    Fatal: MaaLoggingLevel = 1
    Error: MaaLoggingLevel = 2
    Warn: MaaLoggingLevel = 3
    Info: MaaLoggingLevel = 4
    Debug: MaaLoggingLevel = 5
    Trace: MaaLoggingLevel = 6
    All: MaaLoggingLevel = 7


MaaOptionValueSize = ctypes.c_uint64
MaaOptionValue = ctypes.c_void_p

MaaOption = ctypes.c_int32
MaaGlobalOption = MaaOption
MaaCtrlOption = MaaOption


class MaaGlobalOptionEnum:
    Invalid: MaaGlobalOption = 0

    # Log dir
    #
    # value: string, eg: "C:\\Users\\Administrator\\Desktop\\log"; val_size: string length
    LogDir: MaaGlobalOption = 1

    # Whether to save draw
    #
    # value: bool, eg: true; val_size: sizeof(bool)
    SaveDraw: MaaGlobalOption = 2

    # Dump all screenshots and actions
    #
    # Recording will evaluate to true if any of this or MaaCtrlOptionEnum::MaaCtrlOption_Recording
    # is true. value: bool, eg: true; val_size: sizeof(bool)
    Recording: MaaGlobalOption = 3

    # The level of log output to stdout
    #
    # value: MaaLoggingLevel, val_size: sizeof(MaaLoggingLevel)
    # default value is MaaLoggingLevel_Error
    StdoutLevel: MaaGlobalOption = 4

    # Whether to show hit draw
    #
    # value: bool, eg: true; val_size: sizeof(bool)
    ShowHitDraw: MaaGlobalOption = 5

    # Whether to callback debug message
    #
    # value: bool, eg: true; val_size: sizeof(bool)
    DebugMessage: MaaGlobalOption = 6


class MaaCtrlOptionEnum:
    Invalid: MaaCtrlOption = 0

    # Only one of long and short side can be set, and the other is automatically scaled according to the aspect ratio.
    # value: int, eg: 1920; val_size: sizeof(int)
    ScreenshotTargetLongSide: MaaCtrlOption = 1

    # Only one of long and short side can be set, and the other is automatically scaled according to the aspect ratio.
    # value: int, eg: 1080; val_size: sizeof(int)
    ScreenshotTargetShortSide: MaaCtrlOption = 2

    # Dump all screenshots and actions
    # this option will || with MaaGlobalOptionEnum.Recording
    # value: bool, eg: true; val_size: sizeof(bool)
    Recording: MaaCtrlOption = 5


# Use bitwise OR to set the method you need, MaaFramework will test their speed and use the fastest one.
MaaAdbScreencapMethod = ctypes.c_uint64


class MaaAdbScreencapMethodEnum:
    Null: MaaAdbScreencapMethod = 0

    EncodeToFileAndPull: MaaAdbScreencapMethod = 1
    Encode: MaaAdbScreencapMethod = 1 << 1
    RawWithGzip: MaaAdbScreencapMethod = 1 << 2
    RawByNetcat: MaaAdbScreencapMethod = 1 << 3
    MinicapDirect: MaaAdbScreencapMethod = 1 << 4
    MinicapStream: MaaAdbScreencapMethod = 1 << 5
    EmulatorExtras: MaaAdbScreencapMethod = 1 << 6

    All: MaaAdbScreencapMethod = ~Null
    Default: MaaAdbScreencapMethod = All & (~MinicapDirect) & (~MinicapDirect)


# Use bitwise OR to set the method you need, MaaFramework will select the available ones according to priority.
# The priority is: EmulatorExtras > Maatouch > MinitouchAndAdbKey > AdbShell
MaaAdbInputMethod = ctypes.c_uint64


class MaaAdbInputMethodEnum:
    Null: MaaAdbInputMethod = 0

    AdbShell: MaaAdbInputMethod = 1
    MinitouchAndAdbKey: MaaAdbInputMethod = 1 << 1
    Maatouch: MaaAdbInputMethod = 1 << 2
    EmulatorExtras: MaaAdbInputMethod = 1 << 3

    All: MaaAdbInputMethod = ~Null
    Default: MaaAdbInputMethod = All & (~EmulatorExtras)


# No bitwise OR, just set it
MaaWin32ScreencapMethod = ctypes.c_uint64


class MaaWin32ScreencapMethodEnum:
    Null: MaaWin32ScreencapMethod = 0

    GDI: MaaWin32ScreencapMethod = 1
    FramePool: MaaWin32ScreencapMethod = 1 << 1
    DXGI_DesktopDup: MaaWin32ScreencapMethod = 1 << 2


# No bitwise OR, just set it
MaaWin32InputMethod = ctypes.c_uint64


class MaaWin32InputMethodEnum:
    Null: MaaWin32InputMethod = 0

    Seize: MaaWin32InputMethod = 1
    SendMessage: MaaWin32InputMethod = 1 << 1


# No bitwise OR, just set it
MaaDbgControllerType = ctypes.c_uint64


class MaaDbgControllerTypeEnum:
    Null: MaaDbgControllerType = 0

    CarouselImage: MaaDbgControllerType = 1
    ReplayRecording: MaaDbgControllerType = 1 << 1


MaaNotificationCallback = ctypes.CFUNCTYPE(
    None, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_void_p
)

MaaCustomRecognizerCallback = ctypes.CFUNCTYPE(
    MaaBool,  # return value
    MaaContextHandle,  # context
    MaaTaskId,  # task_id
    ctypes.c_char_p,  # current_task
    ctypes.c_char_p,  # custom_recognition_name
    ctypes.c_char_p,  # custom_recognition_param
    MaaImageBufferHandle,  # image
    MaaRectHandle,  # roi
    ctypes.c_void_p,  # trans_arg
    MaaRectHandle,  # [out] out_box
    MaaStringBufferHandle,  # [out] out_detail
)

MaaCustomActionCallback = ctypes.CFUNCTYPE(
    MaaBool,  # return value
    MaaContextHandle,  # context
    MaaTaskId,  # task_id
    ctypes.c_char_p,  # current_task
    ctypes.c_char_p,  # custom_action_name
    ctypes.c_char_p,  #
    MaaRecoId,  # reco_id
    MaaRectHandle,  # box
    ctypes.c_void_p,  # trans_arg
)


MaaToolkitAdbDeviceListHandle = ctypes.c_void_p
MaaToolkitAdbDeviceHandle = ctypes.c_void_p
MaaToolkitDesktopWindowListHandle = ctypes.c_void_p
MaaToolkitDesktopWindowHandle = ctypes.c_void_p

class MaaCustomControllerCallbacks(ctypes.Structure):
    ConnectFunc = ctypes.CFUNCTYPE(
        MaaBool,
        ctypes.c_void_p,
    )
    RequestUuidFunc = ctypes.CFUNCTYPE(
        MaaBool,
        ctypes.c_void_p,
        MaaStringBufferHandle,
    )
    StartAppFunc = ctypes.CFUNCTYPE(
        MaaBool,
        ctypes.c_char_p,
        ctypes.c_void_p,
    )
    StopAppFunc = ctypes.CFUNCTYPE(
        MaaBool,
        ctypes.c_char_p,
        ctypes.c_void_p,
    )
    ScreencapFunc = ctypes.CFUNCTYPE(
        MaaBool,
        ctypes.c_void_p,
        MaaImageBufferHandle,
    )
    ClickFunc = ctypes.CFUNCTYPE(
        MaaBool,
        ctypes.c_int32,
        ctypes.c_int32,
        ctypes.c_void_p,
    )
    SwipeFunc = ctypes.CFUNCTYPE(
        MaaBool,
        ctypes.c_int32,
        ctypes.c_int32,
        ctypes.c_int32,
        ctypes.c_int32,
        ctypes.c_int32,
        ctypes.c_void_p,
    )
    TouchDownFunc = ctypes.CFUNCTYPE(
        MaaBool,
        ctypes.c_int32,
        ctypes.c_int32,
        ctypes.c_int32,
        ctypes.c_int32,
        ctypes.c_void_p,
    )
    TouchMoveFunc = ctypes.CFUNCTYPE(
        MaaBool,
        ctypes.c_int32,
        ctypes.c_int32,
        ctypes.c_int32,
        ctypes.c_int32,
        ctypes.c_void_p,
    )
    TouchUpFunc = ctypes.CFUNCTYPE(
        MaaBool,
        ctypes.c_int32,
        ctypes.c_void_p,
    )
    PressKeyFunc = ctypes.CFUNCTYPE(
        MaaBool,
        ctypes.c_int32,
        ctypes.c_void_p,
    )
    InputTextFunc = ctypes.CFUNCTYPE(
        MaaBool,
        ctypes.c_char_p,
        ctypes.c_void_p,
    )
    _fields_ = [
        ("connect", ConnectFunc),
        ("request_uuid", RequestUuidFunc),
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


@dataclass
class RecognitionDetail:
    reco_id: int
    name: str
    algorithm: str
    box: Optional[Rect]
    detail: Dict
    raw: numpy.ndarray  # only valid in debug mode
    draws: List[numpy.ndarray]  # only valid in debug mode


@dataclass
class NodeDetail:
    node_id: int
    name: str
    recognition: RecognitionDetail
    times: int
    completed: bool


@dataclass
class TaskDetail:
    task_id: int
    entry: str
    nodes: List[NodeDetail]
