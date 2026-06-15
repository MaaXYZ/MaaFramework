import json
from dataclasses import dataclass, field
from typing import Any, Optional, Union, cast

from strenum import StrEnum

# Type aliases to match C++ std::variant types
JRect = tuple[int, int, int, int]  # std::array<int, 4>
JTarget = Union[bool, str, JRect]  # std::variant<bool, std::string, JRect>


# strenum
class JRecognitionType(StrEnum):
    DirectHit = "DirectHit"
    TemplateMatch = "TemplateMatch"
    FeatureMatch = "FeatureMatch"
    ColorMatch = "ColorMatch"
    OCR = "OCR"
    NeuralNetworkClassify = "NeuralNetworkClassify"
    NeuralNetworkDetect = "NeuralNetworkDetect"
    And = "And"
    Or = "Or"
    Custom = "Custom"


class JActionType(StrEnum):
    DoNothing = "DoNothing"
    Click = "Click"
    LongPress = "LongPress"
    Swipe = "Swipe"
    MultiSwipe = "MultiSwipe"
    TouchDown = "TouchDown"
    TouchMove = "TouchMove"
    TouchUp = "TouchUp"
    ClickKey = "ClickKey"
    LongPressKey = "LongPressKey"
    KeyDown = "KeyDown"
    KeyUp = "KeyUp"
    InputText = "InputText"
    StartApp = "StartApp"
    StopApp = "StopApp"
    StopTask = "StopTask"
    Scroll = "Scroll"
    Command = "Command"
    Shell = "Shell"
    Screencap = "Screencap"
    Custom = "Custom"


# Recognition parameter dataclasses (matching C++ JRecognitionParam variants)
@dataclass
class JDirectHit:
    roi: JTarget = (0, 0, 0, 0)
    roi_offset: JRect = (0, 0, 0, 0)


@dataclass
class JTemplateMatch:
    template: list[str]  # 必选
    roi: JTarget = (0, 0, 0, 0)
    roi_offset: JRect = (0, 0, 0, 0)
    threshold: list[float] = field(default_factory=lambda: [0.7])
    order_by: str = "Horizontal"
    index: int = 0
    method: int = 5
    green_mask: bool = False


@dataclass
class JFeatureMatch:
    template: list[str]  # 必选
    roi: JTarget = (0, 0, 0, 0)
    roi_offset: JRect = (0, 0, 0, 0)
    detector: str = "SIFT"
    order_by: str = "Horizontal"
    count: int = 4
    index: int = 0
    green_mask: bool = False
    ratio: float = 0.6


@dataclass
class JColorMatch:
    lower: list[list[int]]  # 必选
    upper: list[list[int]]  # 必选
    roi: JTarget = (0, 0, 0, 0)
    roi_offset: JRect = (0, 0, 0, 0)
    order_by: str = "Horizontal"
    method: int = 4  # RGB
    count: int = 1
    index: int = 0
    connected: bool = False


@dataclass
class JOCR:
    expected: list[str] = field(default_factory=lambda: [])
    roi: JTarget = (0, 0, 0, 0)
    roi_offset: JRect = (0, 0, 0, 0)
    threshold: float = 0.3
    replace: list[list[str]] = field(default_factory=lambda: [])
    order_by: str = "Horizontal"
    index: int = 0
    only_rec: bool = False
    model: str = ""
    color_filter: str = ""


@dataclass
class JNeuralNetworkClassify:
    model: str  # 必选
    expected: list[int] = field(default_factory=lambda: [])
    roi: JTarget = (0, 0, 0, 0)
    roi_offset: JRect = (0, 0, 0, 0)
    labels: list[str] = field(default_factory=lambda: [])
    order_by: str = "Horizontal"
    index: int = 0


@dataclass
class JNeuralNetworkDetect:
    model: str  # 必选
    expected: list[int] = field(default_factory=lambda: [])
    roi: JTarget = (0, 0, 0, 0)
    roi_offset: JRect = (0, 0, 0, 0)
    labels: list[str] = field(default_factory=lambda: [])
    threshold: list[float] = field(default_factory=lambda: [0.3])
    order_by: str = "Horizontal"
    index: int = 0


@dataclass
class JCustomRecognition:
    custom_recognition: str  # 必选
    roi: JTarget = (0, 0, 0, 0)
    roi_offset: JRect = (0, 0, 0, 0)
    custom_recognition_param: Any = None


@dataclass
class JAnd:
    # all_of: List of sub-recognitions. Each element can be:
    #   - str: node name reference (uses that node's recognition params)
    #   - dict/object: inline recognition definition
    all_of: list[Any] = field(default_factory=lambda: [])
    box_index: int = 0


@dataclass
class JOr:
    # any_of: List of sub-recognitions. Each element can be:
    #   - str: node name reference (uses that node's recognition params)
    #   - dict/object: inline recognition definition
    any_of: list[Any] = field(default_factory=lambda: [])


# Recognition parameter union type
JRecognitionParam = Union[
    JDirectHit,
    JTemplateMatch,
    JFeatureMatch,
    JColorMatch,
    JOCR,
    JNeuralNetworkClassify,
    JNeuralNetworkDetect,
    JAnd,
    JOr,
    JCustomRecognition,
]


# Action parameter dataclasses (matching C++ JActionParam variants)
@dataclass
class JDoNothing:
    pass


@dataclass
class JClick:
    target: JTarget = True
    target_offset: JRect = (0, 0, 0, 0)
    contact: int = 0
    pressure: int = 1


@dataclass
class JLongPress:
    target: JTarget = True
    target_offset: JRect = (0, 0, 0, 0)
    duration: int = 1000
    contact: int = 0
    pressure: int = 1


@dataclass
class JSwipe:
    starting: int = 0  # MultiSwipe 中使用
    begin: JTarget = True
    begin_offset: JRect = (0, 0, 0, 0)
    end: list[JTarget] = field(default_factory=lambda: [True])
    end_offset: list[JRect] = field(default_factory=lambda: [(0, 0, 0, 0)])
    end_hold: list[int] = field(default_factory=lambda: [0])
    duration: list[int] = field(default_factory=lambda: [200])
    only_hover: bool = False
    contact: int = 0
    pressure: int = 1


@dataclass
class JMultiSwipe:
    swipes: list[JSwipe]


@dataclass
class JTouch:
    contact: int = 0
    target: JTarget = True
    target_offset: JRect = (0, 0, 0, 0)
    pressure: int = 0


@dataclass
class JTouchUp:
    contact: int = 0


@dataclass
class JClickKey:
    key: list[int]


@dataclass
class JLongPressKey:
    key: list[int]  # 必选
    duration: int = 1000


@dataclass
class JKey:
    key: int


@dataclass
class JInputText:
    input_text: str


@dataclass
class JStartApp:
    package: str


@dataclass
class JStopApp:
    package: str


@dataclass
class JStopTask:
    pass


@dataclass
class JScroll:
    target: JTarget = True
    target_offset: JRect = (0, 0, 0, 0)
    dx: int = 0
    dy: int = 0


@dataclass
class JCommand:
    exec: str  # 必选
    args: list[str] = field(default_factory=lambda: [])
    detach: bool = False


@dataclass
class JShell:
    cmd: str  # 必选
    shell_timeout: int = 20000


@dataclass
class JScreencap:
    filename: str = ""
    format: str = "png"
    quality: int = 100


@dataclass
class JCustomAction:
    custom_action: str  # 必选
    target: JTarget = True
    custom_action_param: Any = None
    target_offset: JRect = (0, 0, 0, 0)


# Action parameter union type
JActionParam = Union[
    JDoNothing,
    JClick,
    JLongPress,
    JSwipe,
    JMultiSwipe,
    JTouch,
    JTouchUp,
    JClickKey,
    JLongPressKey,
    JKey,
    JInputText,
    JStartApp,
    JStopApp,
    JStopTask,
    JScroll,
    JCommand,
    JShell,
    JScreencap,
    JCustomAction,
]


# Main pipeline dataclasses
@dataclass
class JRecognition:
    type: JRecognitionType
    param: JRecognitionParam


@dataclass
class JAction:
    type: JActionType
    param: JActionParam


@dataclass
class JNodeAttr:
    name: str  # 必选
    jump_back: bool = False
    anchor: bool = False


@dataclass
class JWaitFreezes:
    time: int = 1
    target: JTarget = True
    target_offset: JRect = (0, 0, 0, 0)
    threshold: float = 0.95
    method: int = 5
    rate_limit: int = 1000
    timeout: int = 20000


@dataclass
class JPipelineData:
    recognition: JRecognition  # 必选
    action: JAction  # 必选
    next: list[JNodeAttr] = field(default_factory=lambda: [])
    rate_limit: int = 1000
    timeout: int = 20000
    on_error: list[JNodeAttr] = field(default_factory=lambda: [])
    anchor: dict[str, str] = field(default_factory=lambda: {})
    inverse: bool = False
    enabled: bool = True
    pre_delay: int = 200
    post_delay: int = 200
    pre_wait_freezes: Optional[JWaitFreezes] = None
    post_wait_freezes: Optional[JWaitFreezes] = None
    repeat: int = 1
    repeat_delay: int = 0
    repeat_wait_freezes: Optional[JWaitFreezes] = None
    max_hit: int = 4294967295  # UINT_MAX
    focus: Any = None
    attach: dict[str, Any] = field(default_factory=lambda: {})


class JPipelineParser:
    @staticmethod
    def _parse_wait_freezes(data: dict[str, Any]) -> JWaitFreezes:
        """Convert wait freezes with proper defaults"""
        return JWaitFreezes(
            time=cast(int, data.get("time")),
            target=cast(JTarget, data.get("target")),
            target_offset=cast(JRect, data.get("target_offset")),
            threshold=cast(float, data.get("threshold")),
            method=cast(int, data.get("method")),
            rate_limit=cast(int, data.get("rate_limit")),
            timeout=cast(int, data.get("timeout")),
        )

    @classmethod
    def _parse_param(
        cls,
        param_type: Union[JRecognitionType, JActionType],
        param_data: dict[str, Any],
        param_type_map: dict[Any, type[Any]],
    ) -> Union[JRecognitionParam, JActionParam]:
        """Generic function to parse parameters based on type map."""
        param_class = param_type_map.get(param_type)
        if not param_class:
            raise ValueError(f"Unknown type: {param_type}")

        try:
            return param_class(**param_data)
        except TypeError as e:
            print(f"Warning: Failed to create {param_class.__name__} with data {param_data}: {e}")
            return param_class()

    @classmethod
    def _parse_recognition_param(cls, param_type: JRecognitionType, param_data: dict[str, Any]) -> JRecognitionParam:
        """Convert dict to appropriate JRecognitionParam variant based on type."""
        param_type_map = {
            JRecognitionType.DirectHit: JDirectHit,
            JRecognitionType.TemplateMatch: JTemplateMatch,
            JRecognitionType.FeatureMatch: JFeatureMatch,
            JRecognitionType.ColorMatch: JColorMatch,
            JRecognitionType.OCR: JOCR,
            JRecognitionType.NeuralNetworkClassify: JNeuralNetworkClassify,
            JRecognitionType.NeuralNetworkDetect: JNeuralNetworkDetect,
            JRecognitionType.And: JAnd,
            JRecognitionType.Or: JOr,
            JRecognitionType.Custom: JCustomRecognition,
        }
        return cast(JRecognitionParam, cls._parse_param(param_type, param_data, param_type_map))

    @classmethod
    def _parse_action_param(cls, param_type: JActionType, param_data: dict[str, Any]) -> JActionParam:
        """Convert dict to appropriate JActionParam variant based on type."""
        param_type_map = {
            JActionType.DoNothing: JDoNothing,
            JActionType.Click: JClick,
            JActionType.LongPress: JLongPress,
            JActionType.Swipe: JSwipe,
            JActionType.MultiSwipe: JMultiSwipe,
            JActionType.TouchDown: JTouch,
            JActionType.TouchMove: JTouch,
            JActionType.TouchUp: JTouchUp,
            JActionType.ClickKey: JClickKey,
            JActionType.LongPressKey: JLongPressKey,
            JActionType.KeyDown: JKey,
            JActionType.KeyUp: JKey,
            JActionType.InputText: JInputText,
            JActionType.StartApp: JStartApp,
            JActionType.StopApp: JStopApp,
            JActionType.StopTask: JStopTask,
            JActionType.Scroll: JScroll,
            JActionType.Command: JCommand,
            JActionType.Shell: JShell,
            JActionType.Screencap: JScreencap,
            JActionType.Custom: JCustomAction,
        }

        return cast(JActionParam, cls._parse_param(param_type, param_data, param_type_map))

    @classmethod
    def parse_pipeline_data(cls, pipeline_data: Union[str, dict[str, Any]]) -> JPipelineData:
        """Parse JSON string to JPipelineData dataclass with proper variant types."""
        if isinstance(pipeline_data, dict):
            data = pipeline_data
        else:
            try:
                data = cast(dict[str, Any], json.loads(pipeline_data))
            except json.JSONDecodeError as e:
                raise ValueError(f"Invalid JSON format: {e}") from e

        # Convert recognition
        recognition_data = cast(dict[str, Any], data.get("recognition"))
        recognition_type: JRecognitionType = JRecognitionType(recognition_data.get("type"))
        recognition_param_data = cast(dict[str, Any], recognition_data.get("param"))
        recognition_param = cls._parse_recognition_param(recognition_type, recognition_param_data)
        recognition = JRecognition(type=recognition_type, param=recognition_param)

        # Convert action
        action_data = cast(dict[str, Any], data.get("action"))
        action_type: JActionType = JActionType(action_data.get("type"))
        action_param_data = cast(dict[str, Any], action_data.get("param"))
        action_param = cls._parse_action_param(action_type, action_param_data)
        action = JAction(type=action_type, param=action_param)

        pre_wait_freezes = cls._parse_wait_freezes(cast(dict[str, Any], data.get("pre_wait_freezes")))
        post_wait_freezes = cls._parse_wait_freezes(cast(dict[str, Any], data.get("post_wait_freezes")))
        repeat_wait_freezes = cls._parse_wait_freezes(cast(dict[str, Any], data.get("repeat_wait_freezes")))

        # Create JPipelineData with converted data
        return JPipelineData(
            recognition=recognition,
            action=action,
            next=cls._parse_node_attr_list(cast(list[dict[str, Any]], data.get("next"))),
            rate_limit=cast(int, data.get("rate_limit")),
            timeout=cast(int, data.get("timeout")),
            on_error=cls._parse_node_attr_list(cast(list[dict[str, Any]], data.get("on_error"))),
            anchor=cast(dict[str, str], data.get("anchor", {})),
            inverse=cast(bool, data.get("inverse")),
            enabled=cast(bool, data.get("enabled")),
            pre_delay=cast(int, data.get("pre_delay")),
            post_delay=cast(int, data.get("post_delay")),
            pre_wait_freezes=pre_wait_freezes,
            post_wait_freezes=post_wait_freezes,
            repeat=cast(int, data.get("repeat")),
            repeat_delay=cast(int, data.get("repeat_delay")),
            repeat_wait_freezes=repeat_wait_freezes,
            max_hit=cast(int, data.get("max_hit")),
            focus=data.get("focus"),
            attach=cast(dict[str, Any], data.get("attach")),
        )

    @staticmethod
    def _parse_node_attr_list(data: list[dict[str, Any]]) -> list[JNodeAttr]:
        """Convert list of dicts to list of JNodeAttr."""
        return [
            JNodeAttr(
                name=cast(str, item.get("name")),
                jump_back=cast(bool, item.get("jump_back", False)),
                anchor=cast(bool, item.get("anchor", False)),
            )
            for item in data
        ]
