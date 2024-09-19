import ctypes
import platform
from dataclasses import dataclass
from enum import IntEnum, Enum
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


class MaaStatusEnum(IntEnum):
    invalid = 0
    pending = 1000
    running = 2000
    succeeded = 3000
    failed = 4000


MaaLoggingLevel = ctypes.c_int32


MaaOptionValueSize = ctypes.c_uint64
MaaOptionValue = ctypes.c_void_p

MaaOption = ctypes.c_int32
MaaGlobalOption = MaaOption
MaaCtrlOption = MaaOption


class MaaGlobalOptionEnum:
    Invalid = 0

    # Log dir
    #
    # value: string, eg: "C:\\Users\\Administrator\\Desktop\\log"; val_size: string length
    LogDir = 1

    # Whether to save draw
    #
    # value: bool, eg: true; val_size: sizeof(bool)
    SaveDraw = 2

    # Dump all screenshots and actions
    #
    # Recording will evaluate to true if any of this or MaaCtrlOptionEnum::MaaCtrlOption_Recording
    # is true. value: bool, eg: true; val_size: sizeof(bool)
    Recording = 3

    # The level of log output to stdout
    #
    # value, val_size: sizeof(MaaLoggingLevel)
    # default value is MaaLoggingLevel_Error
    StdoutLevel = 4

    # Whether to show hit draw
    #
    # value: bool, eg: true; val_size: sizeof(bool)
    ShowHitDraw = 5

    # Whether to debug
    #
    # value: bool, eg: true; val_size: sizeof(bool)
    DebugMode = 6


class MaaCtrlOptionEnum:
    Invalid = 0

    # Only one of long and short side can be set, and the other is automatically scaled according to the aspect ratio.
    # value: int, eg: 1920; val_size: sizeof(int)
    ScreenshotTargetLongSide = 1

    # Only one of long and short side can be set, and the other is automatically scaled according to the aspect ratio.
    # value: int, eg: 1080; val_size: sizeof(int)
    ScreenshotTargetShortSide = 2

    # Dump all screenshots and actions
    # this option will || with MaaGlobalOptionEnum.Recording
    # value: bool, eg: true; val_size: sizeof(bool)
    Recording = 5


MaaAdbScreencapMethod = ctypes.c_uint64


class MaaAdbScreencapMethodEnum:
    """
    Use bitwise OR to set the method you need
    MaaFramework will test their speed and use the fastest one.
    """

    Null = 0

    EncodeToFileAndPull = 1
    Encode = 1 << 1
    RawWithGzip = 1 << 2
    RawByNetcat = 1 << 3
    MinicapDirect = 1 << 4
    MinicapStream = 1 << 5
    EmulatorExtras = 1 << 6

    All = ~Null
    Default = All & (~MinicapDirect) & (~MinicapDirect)


MaaAdbInputMethod = ctypes.c_uint64


class MaaAdbInputMethodEnum:
    """
    Use bitwise OR to set the method you need
    MaaFramework will select the available ones according to priority.
    The priority is: EmulatorExtras > Maatouch > MinitouchAndAdbKey > AdbShell
    """

    Null = 0

    AdbShell = 1
    MinitouchAndAdbKey = 1 << 1
    Maatouch = 1 << 2
    EmulatorExtras = 1 << 3

    All = ~Null
    Default = All & (~EmulatorExtras)


MaaWin32ScreencapMethod = ctypes.c_uint64


# No bitwise OR, just set it
class MaaWin32ScreencapMethodEnum:
    Null = 0

    GDI = 1
    FramePool = 1 << 1
    DXGI_DesktopDup = 1 << 2


MaaWin32InputMethod = ctypes.c_uint64


# No bitwise OR, just set it
class MaaWin32InputMethodEnum:
    Null = 0

    Seize = 1
    SendMessage = 1 << 1


# No bitwise OR, just set it
MaaDbgControllerType = ctypes.c_uint64


class MaaDbgControllerTypeEnum:
    Null = 0

    CarouselImage = 1
    ReplayRecording = 1 << 1


FUNCTYPE = ctypes.WINFUNCTYPE if (platform.system() == "Windows") else ctypes.CFUNCTYPE

MaaNotificationCallback = FUNCTYPE(
    None, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_void_p
)

MaaCustomRecognitionCallback = FUNCTYPE(
    MaaBool,  # return value
    MaaContextHandle,  # context
    MaaTaskId,  # task_id
    ctypes.c_char_p,  # current_task_name
    ctypes.c_char_p,  # custom_recognition_name
    ctypes.c_char_p,  # custom_recognition_param
    MaaImageBufferHandle,  # image
    MaaRectHandle,  # roi
    ctypes.c_void_p,  # trans_arg
    MaaRectHandle,  # [out] out_box
    MaaStringBufferHandle,  # [out] out_detail
)

MaaCustomActionCallback = FUNCTYPE(
    MaaBool,  # return value
    MaaContextHandle,  # context
    MaaTaskId,  # task_id
    ctypes.c_char_p,  # current_task_name
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
    ConnectFunc = FUNCTYPE(
        MaaBool,
        ctypes.c_void_p,
    )
    RequestUuidFunc = FUNCTYPE(
        MaaBool,
        ctypes.c_void_p,
        MaaStringBufferHandle,
    )
    StartAppFunc = FUNCTYPE(
        MaaBool,
        ctypes.c_char_p,
        ctypes.c_void_p,
    )
    StopAppFunc = FUNCTYPE(
        MaaBool,
        ctypes.c_char_p,
        ctypes.c_void_p,
    )
    ScreencapFunc = FUNCTYPE(
        MaaBool,
        ctypes.c_void_p,
        MaaImageBufferHandle,
    )
    ClickFunc = FUNCTYPE(
        MaaBool,
        ctypes.c_int32,
        ctypes.c_int32,
        ctypes.c_void_p,
    )
    SwipeFunc = FUNCTYPE(
        MaaBool,
        ctypes.c_int32,
        ctypes.c_int32,
        ctypes.c_int32,
        ctypes.c_int32,
        ctypes.c_int32,
        ctypes.c_void_p,
    )
    TouchDownFunc = FUNCTYPE(
        MaaBool,
        ctypes.c_int32,
        ctypes.c_int32,
        ctypes.c_int32,
        ctypes.c_int32,
        ctypes.c_void_p,
    )
    TouchMoveFunc = FUNCTYPE(
        MaaBool,
        ctypes.c_int32,
        ctypes.c_int32,
        ctypes.c_int32,
        ctypes.c_int32,
        ctypes.c_void_p,
    )
    TouchUpFunc = FUNCTYPE(
        MaaBool,
        ctypes.c_int32,
        ctypes.c_void_p,
    )
    PressKeyFunc = FUNCTYPE(
        MaaBool,
        ctypes.c_int32,
        ctypes.c_void_p,
    )
    InputTextFunc = FUNCTYPE(
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


class AlgorithmEnum(str, Enum):
    DirectHit = "DirectHit"
    TemplateMatch = "TemplateMatch"
    FeatureMatch = "FeatureMatch"
    ColorMatch = "ColorMatch"
    OCR = "OCR"
    NeuralNetworkClassify = "NeuralNetworkClassify"
    NeuralNetworkDetect = "NeuralNetworkDetect"
    Custom = "Custom"


@dataclass
class BoxAndScoreResult:
    box: Rect
    score: float


TemplateMatchResult = BoxAndScoreResult


@dataclass
class BoxAndCountResult:
    box: Rect
    count: int


FeatureMatchResult = BoxAndCountResult
ColorMatchResult = BoxAndCountResult


@dataclass
class OCRResult(BoxAndScoreResult):
    text: str


@dataclass
class NeuralNetworkResult(BoxAndScoreResult):
    cls_index: int
    label: str
    box: Rect
    score: float


NeuralNetworkClassifyResult = NeuralNetworkResult
NeuralNetworkDetectResult = NeuralNetworkResult


@dataclass
class CustomRecognitionResult:
    box: Rect
    detail: Union[str, Dict]


RecognitionResult = Union[
    TemplateMatchResult,
    FeatureMatchResult,
    ColorMatchResult,
    OCRResult,
    NeuralNetworkClassifyResult,
    NeuralNetworkDetectResult,
    CustomRecognitionResult,
]

AlgorithmResultDict = {
    AlgorithmEnum.DirectHit: None,
    AlgorithmEnum.TemplateMatch: TemplateMatchResult,
    AlgorithmEnum.FeatureMatch: FeatureMatchResult,
    AlgorithmEnum.ColorMatch: ColorMatchResult,
    AlgorithmEnum.OCR: OCRResult,
    AlgorithmEnum.NeuralNetworkClassify: NeuralNetworkClassifyResult,
    AlgorithmEnum.NeuralNetworkDetect: NeuralNetworkDetectResult,
    AlgorithmEnum.Custom: CustomRecognitionResult,
}


@dataclass
class RecognitionDetail:
    reco_id: int
    name: str
    algorithm: AlgorithmEnum
    box: Optional[Rect]

    all_results: List[RecognitionResult]
    filterd_results: List[RecognitionResult]
    best_result: Optional[RecognitionResult]

    raw_detail: Dict
    raw_image: numpy.ndarray  # only valid in debug mode
    draw_images: List[numpy.ndarray]  # only valid in debug mode


@dataclass
class NodeDetail:
    node_id: int
    name: str
    recognition: RecognitionDetail
    completed: bool


@dataclass
class TaskDetail:
    task_id: int
    entry: str
    nodes: List[NodeDetail]


class LoggingLevelEnum(IntEnum):
    Off = 0
    Fatal = 1
    Error = 2
    Warn = 3
    Info = 4
    Debug = 5
    Trace = 6
    All = 7
