import json
from dataclasses import dataclass
from typing import Any, List, Tuple, Union, Dict

# Type aliases to match C++ std::variant types
JRect = Tuple[int, int, int, int]  # std::array<int, 4>
JTarget = Union[bool, str, JRect]  # std::variant<bool, std::string, JRect>


# Recognition parameter dataclasses (matching C++ JRecognitionParam variants)
@dataclass
class JDirectHit:
    pass


@dataclass
class JTemplateMatch:
    roi: JTarget
    roi_offset: JRect
    template: List[str]
    threshold: List[float]
    order_by: str
    index: int
    method: int
    green_mask: bool


@dataclass
class JFeatureMatch:
    roi: JTarget
    roi_offset: JRect
    template: List[str]
    detector: str
    order_by: str
    count: int
    index: int
    green_mask: bool
    ratio: float


@dataclass
class JColorMatch:
    roi: JTarget
    roi_offset: JRect
    lower: List[List[int]]
    upper: List[List[int]]
    order_by: str
    method: int
    count: int
    index: int
    connected: bool


@dataclass
class JOCR:
    roi: JTarget
    roi_offset: JRect
    expected: List[str]
    threshold: float
    replace: List[List[str]]
    order_by: str
    index: int
    only_rec: bool
    model: str


@dataclass
class JNeuralNetworkClassify:
    roi: JTarget
    roi_offset: JRect
    labels: List[str]
    model: str
    expected: List[int]
    order_by: str
    index: int


@dataclass
class JNeuralNetworkDetect:
    roi: JTarget
    roi_offset: JRect
    labels: List[str]
    model: str
    expected: List[int]
    threshold: List[float]
    order_by: str
    index: int


@dataclass
class JCustomRecognition:
    roi: JTarget
    roi_offset: JRect
    custom_recognition: str
    custom_recognition_param: Any


# Recognition parameter union type
JRecognitionParam = Union[
    JDirectHit,
    JTemplateMatch,
    JFeatureMatch,
    JColorMatch,
    JOCR,
    JNeuralNetworkClassify,
    JNeuralNetworkDetect,
    JCustomRecognition,
]


# Action parameter dataclasses (matching C++ JActionParam variants)
@dataclass
class JDoNothing:
    pass


@dataclass
class JClick:
    target: JTarget
    target_offset: JRect
    contact: int = 0


@dataclass
class JLongPress:
    target: JTarget
    target_offset: JRect
    duration: int
    contact: int = 0


@dataclass
class JSwipe:
    starting: int
    begin: JTarget
    begin_offset: JRect
    end: List[JTarget]
    end_offset: List[JRect]
    end_hold: List[int]
    duration: List[int]
    only_hover: bool
    contact: int = 0


@dataclass
class JMultiSwipe:
    swipes: List[JSwipe]


@dataclass
class JTouch:
    contact: int
    target: JTarget
    target_offset: JRect
    pressure: int


@dataclass
class JTouchUp:
    contact: int


@dataclass
class JClickKey:
    key: List[int]


@dataclass
class JLongPressKey:
    key: List[int]
    duration: int


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
class JCommand:
    exec: str
    args: List[str]
    detach: bool


@dataclass
class JCustomAction:
    target: JTarget
    custom_action: str
    custom_action_param: Any
    target_offset: JRect


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
    JCommand,
    JCustomAction,
]


# Main pipeline dataclasses
@dataclass
class JRecognition:
    type: str
    param: JRecognitionParam


@dataclass
class JAction:
    type: str
    param: JActionParam


@dataclass
class JWaitFreezes:
    time: int
    target: JTarget
    target_offset: JRect
    threshold: float
    method: int
    rate_limit: int
    timeout: int


@dataclass
class JPipelineData:
    recognition: JRecognition
    action: JAction
    next: List[str]
    interrupt: List[str]
    is_sub: bool
    rate_limit: int
    timeout: int
    on_error: List[str]
    inverse: bool
    enabled: bool
    pre_delay: int
    post_delay: int
    pre_wait_freezes: JWaitFreezes
    post_wait_freezes: JWaitFreezes
    focus: Any
    attach: Dict  # 附加 JSON 对象


class JPipelineParser:
    @staticmethod
    def _parse_wait_freezes(data: dict) -> JWaitFreezes:
        """Convert wait freezes with proper defaults"""
        return JWaitFreezes(
            time=data.get("time"),
            target=data.get("target"),  # type: ignore
            target_offset=data.get("target_offset"),  # type: ignore
            threshold=data.get("threshold"),
            method=data.get("method"),
            rate_limit=data.get("rate_limit"),
            timeout=data.get("timeout"),
        )

    @classmethod
    def _parse_param(
        cls, param_type: str, param_data: dict, param_type_map: dict, default_class
    ):
        """Generic function to parse parameters based on type map."""
        param_class = param_type_map.get(param_type)
        if not param_class:
            raise ValueError(f"Unknown type: {param_type}")

        if param_class == default_class:
            return param_class()

        try:
            return param_class(**param_data)
        except TypeError as e:
            print(
                f"Warning: Failed to create {param_class.__name__} with data {param_data}: {e}"
            )
            return default_class()

    @classmethod
    def _parse_recognition_param(
        cls, param_type: str, param_data: dict
    ) -> JRecognitionParam:
        """Convert dict to appropriate JRecognitionParam variant based on type."""
        param_type_map = {
            "DirectHit": JDirectHit,
            "TemplateMatch": JTemplateMatch,
            "FeatureMatch": JFeatureMatch,
            "ColorMatch": JColorMatch,
            "OCR": JOCR,
            "NeuralNetworkClassify": JNeuralNetworkClassify,
            "NeuralNetworkDetect": JNeuralNetworkDetect,
            "Custom": JCustomRecognition,
        }
        return cls._parse_param(param_type, param_data, param_type_map, JDirectHit)

    @classmethod
    def _parse_action_param(cls, param_type: str, param_data: dict) -> JActionParam:
        """Convert dict to appropriate JActionParam variant based on type."""
        param_type_map = {
            "DoNothing": JDoNothing,
            "Click": JClick,
            "LongPress": JLongPress,
            "Swipe": JSwipe,
            "MultiSwipe": JMultiSwipe,
            "TouchDown": JTouch,
            "TouchMove": JTouch,
            "TouchUp": JTouchUp,
            "ClickKey": JClickKey,
            "LongPressKey": JLongPressKey,
            "KeyDown": JKey,
            "KeyUp": JKey,
            "InputText": JInputText,
            "StartApp": JStartApp,
            "StopApp": JStopApp,
            "StopTask": JStopTask,
            "Command": JCommand,
            "Custom": JCustomAction,
        }

        return cls._parse_param(param_type, param_data, param_type_map, JDoNothing)

    @classmethod
    def parse_pipeline_data(cls, pipeline_data: Union[str, Dict]) -> JPipelineData:
        """Parse JSON string to JPipelineData dataclass with proper variant types."""
        if isinstance(pipeline_data, dict):
            data = pipeline_data
        elif isinstance(pipeline_data, str):
            try:
                data: dict = json.loads(pipeline_data)
            except json.JSONDecodeError as e:
                raise ValueError(f"Invalid JSON format: {e}")
        else:
            raise TypeError("Input must be a JSON string or a dict.")

        # Convert recognition
        recognition_data: dict = data.get("recognition")
        recognition_type: str = recognition_data.get("type")
        recognition_param_data: dict = recognition_data.get("param")
        recognition_param = cls._parse_recognition_param(
            recognition_type, recognition_param_data
        )
        recognition = JRecognition(type=recognition_type, param=recognition_param)

        # Convert action
        action_data: dict = data.get("action")
        action_type: str = action_data.get("type")
        action_param_data = action_data.get("param")
        action_param = cls._parse_action_param(action_type, action_param_data)
        action = JAction(type=action_type, param=action_param)

        pre_wait_freezes = cls._parse_wait_freezes(data.get("pre_wait_freezes"))  # type: ignore
        post_wait_freezes = cls._parse_wait_freezes(data.get("post_wait_freezes"))  # type: ignore

        # Create JPipelineData with converted data
        return JPipelineData(
            recognition=recognition,
            action=action,
            next=data.get("next"),
            interrupt=data.get("interrupt"),
            is_sub=data.get("is_sub"),
            rate_limit=data.get("rate_limit"),
            timeout=data.get("timeout"),
            on_error=data.get("on_error"),
            inverse=data.get("inverse"),
            enabled=data.get("enabled"),
            pre_delay=data.get("pre_delay"),
            post_delay=data.get("post_delay"),
            pre_wait_freezes=pre_wait_freezes,  # type: ignore
            post_wait_freezes=post_wait_freezes,  # type: ignore
            focus=data.get("focus"),
            attach=data.get("attach"),  # 附加 JSON 对象
        )
