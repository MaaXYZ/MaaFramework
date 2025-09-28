import ctypes
import pathlib
import json
from typing import Any, Optional, Union, List, Dict
from dataclasses import dataclass, field, asdict

from .notification_handler import NotificationHandler
from .define import *
from .job import Job
from .library import Library
from .buffer import StringBuffer, StringListBuffer


# Type aliases to match C++ std::variant types
JRect = List[int]  # std::array<int, 4>
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
    index: int = 0
    method: int = 0
    green_mask: bool = False


@dataclass
class JFeatureMatch:
    roi: JTarget
    roi_offset: JRect
    template: List[str]
    detector: str
    order_by: str
    count: int = 0
    index: int = 0
    green_mask: bool = False
    ratio: float = 0


@dataclass
class JColorMatch:
    roi: JTarget
    roi_offset: JRect
    lower: List[List[int]]
    upper: List[List[int]]
    order_by: str
    method: int = 0
    count: int = 0
    index: int = 0
    connected: bool = False


@dataclass
class JOCR:
    roi: JTarget
    roi_offset: JRect
    expected: List[str] = field(default_factory=list)
    threshold: float = 0
    replace: List[List[str]] = field(default_factory=list)
    order_by: str = ""
    index: int = 0
    only_rec: bool = False
    model: str = ""


@dataclass
class JNeuralNetworkClassify:
    roi: JTarget
    roi_offset: JRect
    labels: List[str]
    model: str
    expected: List[int] = field(default_factory=list)
    order_by: str = ""
    index: int = 0


@dataclass
class JNeuralNetworkDetect:
    roi: JTarget
    roi_offset: JRect
    labels: List[str]
    model: str
    expected: List[int] = field(default_factory=list)
    threshold: List[float] = field(default_factory=list)
    order_by: str = ""
    index: int = 0


@dataclass
class JCustomRecognition:
    roi: JTarget
    roi_offset: JRect
    custom_recognition: str
    custom_recognition_param: Any


# Recognition parameter union type
JRecognitionParam = Union[
    JDirectHit, JTemplateMatch, JFeatureMatch, JColorMatch, JOCR,
    JNeuralNetworkClassify, JNeuralNetworkDetect, JCustomRecognition
]


# Action parameter dataclasses (matching C++ JActionParam variants)
@dataclass
class JDoNothing:
    pass


@dataclass
class JClick:
    target: JTarget
    target_offset: JRect = field(default_factory=lambda: [0, 0, 0, 0])


@dataclass
class JLongPress:
    target: JTarget
    target_offset: JRect = field(default_factory=lambda: [0, 0, 0, 0])
    duration: int = 0


@dataclass
class JSwipe:
    starting: int = 0
    begin: Optional[JTarget] = None
    begin_offset: JRect = field(default_factory=lambda: [0, 0, 0, 0])
    end: List[JTarget] = field(default_factory=list)
    end_offset: List[JRect] = field(default_factory=list)
    end_hold: List[int] = field(default_factory=list)
    duration: List[int] = field(default_factory=list)
    only_hover: bool = False


@dataclass
class JMultiSwipe:
    swipes: List[JSwipe] = field(default_factory=list)


@dataclass
class JClickKey:
    key: List[int] = field(default_factory=list)


@dataclass
class JLongPressKey:
    key: List[int] = field(default_factory=list)
    duration: int = 0


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
    args: List[str] = field(default_factory=list)
    detach: bool = False


@dataclass
class JCustomAction:
    target: JTarget
    custom_action: str
    custom_action_param: Any
    target_offset: JRect = field(default_factory=lambda: [0, 0, 0, 0])


# Action parameter union type
JActionParam = Union[
    JDoNothing, JClick, JLongPress, JSwipe, JMultiSwipe, JClickKey,
    JLongPressKey, JInputText, JStartApp, JStopApp, JStopTask, JCommand, JCustomAction
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
    time: int = 0
    target: Optional[JTarget] = None
    target_offset: Optional[JRect] = None
    threshold: float = 0
    method: int = 0
    rate_limit: int = 0
    timeout: int = 0


@dataclass
class JPipelineData:
    recognition: JRecognition
    action: JAction
    next: List[str] = field(default_factory=list)
    interrupt: List[str] = field(default_factory=list)
    is_sub: bool = False
    rate_limit: int = 0
    timeout: int = 0
    on_error: List[str] = field(default_factory=list)
    inverse: bool = False
    enabled: bool = False
    pre_delay: int = 0
    post_delay: int = 0
    pre_wait_freezes: Optional[JWaitFreezes] = None
    post_wait_freezes: Optional[JWaitFreezes] = None
    focus: Optional[Any] = None


# Export public functions and classes
__all__ = [
    'Resource',
    'JPipelineData', 'JRecognition', 'JAction', 'JWaitFreezes',
    'JDirectHit', 'JTemplateMatch', 'JFeatureMatch', 'JColorMatch', 'JOCR', 
    'JNeuralNetworkClassify', 'JNeuralNetworkDetect', 'JCustomRecognition',
    'JDoNothing', 'JClick', 'JLongPress', 'JSwipe', 'JMultiSwipe', 'JClickKey',
    'JLongPressKey', 'JInputText', 'JStartApp', 'JStopApp', 'JStopTask', 'JCommand', 'JCustomAction',
    'parse_pipeline_data', 'dump_pipeline_data'
]


def parse_recognition_param(param_type: str, param_data: dict) -> JRecognitionParam:
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
    
    param_class = param_type_map.get(param_type)
    if not param_class:
        raise ValueError(f"Unknown recognition type: {param_type}")
    
    if param_class == JDirectHit:
        return JDirectHit()
    
    # Convert dict to dataclass, handling default values and field mappings
    field_names = {f.name for f in param_class.__dataclass_fields__.values()}
    filtered_data = {}
    
    for key, value in param_data.items():
        # Handle special field name mappings from C++
        if key == "template_" and "template" in field_names:
            filtered_data["template"] = value
        elif key in field_names:
            filtered_data[key] = value
    
    try:
        return param_class(**filtered_data)
    except TypeError as e:
        # Handle missing required fields by providing reasonable defaults
        print(f"Warning: Failed to create {param_class.__name__} with data {filtered_data}: {e}")
        # For now, return DirectHit as a fallback
        return JDirectHit()


def parse_action_param(param_type: str, param_data: dict) -> JActionParam:
    """Convert dict to appropriate JActionParam variant based on type."""
    param_type_map = {
        "DoNothing": JDoNothing,
        "Click": JClick,
        "LongPress": JLongPress,
        "Swipe": JSwipe,
        "MultiSwipe": JMultiSwipe,
        "ClickKey": JClickKey,
        "Key": JClickKey,  # Alias for ClickKey
        "LongPressKey": JLongPressKey,
        "InputText": JInputText,
        "StartApp": JStartApp,
        "StopApp": JStopApp,
        "StopTask": JStopTask,
        "Command": JCommand,
        "Custom": JCustomAction,
    }
    
    param_class = param_type_map.get(param_type)
    if not param_class:
        raise ValueError(f"Unknown action type: {param_type}")
    
    if param_class in (JDoNothing, JStopTask):
        return param_class()
    
    # Convert dict to dataclass, handling default values
    field_names = {f.name for f in param_class.__dataclass_fields__.values()}
    filtered_data = {k: v for k, v in param_data.items() if k in field_names}
    
    try:
        return param_class(**filtered_data)
    except TypeError as e:
        # Handle missing required fields by providing reasonable defaults
        print(f"Warning: Failed to create {param_class.__name__} with data {filtered_data}: {e}")
        # For now, return DoNothing as a fallback
        return JDoNothing()


def parse_pipeline_data(json_str: str) -> JPipelineData:
    """Parse JSON string to JPipelineData dataclass with proper variant types."""
    try:
        data = json.loads(json_str)
    except json.JSONDecodeError as e:
        raise ValueError(f"Invalid JSON format: {e}")
    
    # Convert recognition
    recognition_data = data.get("recognition", {})
    recognition_type = recognition_data.get("type", "")
    recognition_param_data = recognition_data.get("param", {})
    recognition_param = parse_recognition_param(recognition_type, recognition_param_data)
    recognition = JRecognition(type=recognition_type, param=recognition_param)
    
    # Convert action
    action_data = data.get("action", {})
    action_type = action_data.get("type", "")
    action_param_data = action_data.get("param", {})
    action_param = parse_action_param(action_type, action_param_data)
    action = JAction(type=action_type, param=action_param)
    
    # Convert wait freezes with proper defaults
    def create_wait_freezes(data_dict):
        if not data_dict:
            return None
        return JWaitFreezes(
            time=data_dict.get("time", 0),
            target=data_dict.get("target"),
            target_offset=data_dict.get("target_offset"),
            threshold=data_dict.get("threshold", 0),
            method=data_dict.get("method", 0),
            rate_limit=data_dict.get("rate_limit", 0),
            timeout=data_dict.get("timeout", 0),
        )
    
    pre_wait_freezes = create_wait_freezes(data.get("pre_wait_freezes"))
    post_wait_freezes = create_wait_freezes(data.get("post_wait_freezes"))
    
    # Create JPipelineData with converted data
    return JPipelineData(
        recognition=recognition,
        action=action,
        next=data.get("next", []),
        interrupt=data.get("interrupt", []),
        is_sub=data.get("is_sub", False),
        rate_limit=data.get("rate_limit", 0),
        timeout=data.get("timeout", 0),
        on_error=data.get("on_error", []),
        inverse=data.get("inverse", False),
        enabled=data.get("enabled", False),
        pre_delay=data.get("pre_delay", 0),
        post_delay=data.get("post_delay", 0),
        pre_wait_freezes=pre_wait_freezes,
        post_wait_freezes=post_wait_freezes,
        focus=data.get("focus"),
    )


def dump_pipeline_data(pipeline_data: JPipelineData) -> str:
    """Convert JPipelineData dataclass back to JSON string for C++ interop."""
    
    # Use asdict() to convert the entire dataclass to dict
    result = asdict(pipeline_data)
    
    # Handle special field name mappings for recognition params
    if "recognition" in result and "param" in result["recognition"]:
        recognition_type = result["recognition"].get("type", "")
        param_dict = result["recognition"]["param"]
        if recognition_type in ["TemplateMatch", "FeatureMatch"] and "template" in param_dict:
            param_dict["template_"] = param_dict.pop("template")
    
    # Clean up the result: remove None values, empty lists, and default values
    def clean_dict(d):
        if isinstance(d, dict):
            cleaned = {}
            for k, v in d.items():
                # Clean nested values and skip empty/default values
                cleaned_v = clean_dict(v)
                if cleaned_v is not None and cleaned_v != [] and cleaned_v != {}:
                    # Include non-zero numbers and True booleans
                    if cleaned_v != 0 and cleaned_v != False:
                        cleaned[k] = cleaned_v
                    elif cleaned_v is True or (isinstance(cleaned_v, (int, float)) and cleaned_v != 0):
                        cleaned[k] = cleaned_v
            return cleaned if cleaned else None
        elif isinstance(d, list):
            cleaned_list = [clean_dict(item) for item in d if item is not None]
            return cleaned_list if cleaned_list else None
        else:
            return d
    
    cleaned_result = clean_dict(result)
    final_dict = cleaned_result if cleaned_result else {}
    
    # Convert to JSON string
    return json.dumps(final_dict, ensure_ascii=False)



class Resource:
    _notification_handler: Optional[NotificationHandler]
    _handle: MaaResourceHandle
    _own: bool

    ### public ###

    def __init__(
        self,
        notification_handler: Optional[NotificationHandler] = None,
        handle: Optional[MaaResourceHandle] = None,
    ):
        self._set_api_properties()

        if handle:
            self._handle = handle
            self._own = False
        else:
            self._notification_handler = notification_handler
            self._handle = Library.framework().MaaResourceCreate(
                *NotificationHandler._gen_c_param(self._notification_handler)
            )
            self._own = True

        if not self._handle:
            raise RuntimeError("Failed to create resource.")

        self._custom_action_holder = {}
        self._custom_recognition_holder = {}

    def __del__(self):
        if self._handle and self._own:
            Library.framework().MaaResourceDestroy(self._handle)

    def post_bundle(self, path: Union[pathlib.Path, str]) -> Job:
        resid = Library.framework().MaaResourcePostBundle(
            self._handle, str(path).encode()
        )
        return Job(resid, self._status, self._wait)

    def override_pipeline(self, pipeline_override: Union[Dict, JPipelineData]) -> bool:
        if isinstance(pipeline_override, JPipelineData):
            pipeline_json = dump_pipeline_data(pipeline_override)
        else:
            pipeline_json = json.dumps(pipeline_override, ensure_ascii=False)
            
        return bool(
            Library.framework().MaaResourceOverridePipeline(
                self._handle,
                pipeline_json.encode(),
            )
        )

    def override_next(self, name: str, next_list: List[str]) -> bool:
        list_buffer = StringListBuffer()
        list_buffer.set(next_list)

        return bool(
            Library.framework().MaaResourceOverrideNext(
                self._handle, name.encode(), list_buffer._handle
            )
        )
    
    def get_node_data(self, name: str) -> Optional[JPipelineData]:
        string_buffer = StringBuffer()
        if not Library.framework().MaaResourceGetNodeData(
            self._handle, name.encode(), string_buffer._handle
        ):
            return None

        data = string_buffer.get()
        if not data:
            return None

        try:
            return parse_pipeline_data(data)
        except (ValueError, TypeError) as e:
            # Log error for debugging but return None for backward compatibility
            return None

    @property
    def loaded(self) -> bool:
        return bool(Library.framework().MaaResourceLoaded(self._handle))

    def clear(self) -> bool:
        return bool(Library.framework().MaaResourceClear(self._handle))

    def use_cpu(self) -> bool:
        return self.set_inference(
            MaaInferenceExecutionProviderEnum.CPU, MaaInferenceDeviceEnum.CPU
        )

    def use_directml(self, device_id: int = MaaInferenceDeviceEnum.Auto) -> bool:
        return self.set_inference(MaaInferenceExecutionProviderEnum.DirectML, device_id)

    def use_coreml(self, coreml_flag: int = MaaInferenceDeviceEnum.Auto) -> bool:
        return self.set_inference(MaaInferenceExecutionProviderEnum.CoreML, coreml_flag)

    def use_auto_ep(self) -> bool:
        return self.set_inference(
            MaaInferenceExecutionProviderEnum.Auto, MaaInferenceDeviceEnum.Auto
        )

    # not implemented
    # def use_cuda(self, nvidia_gpu_id: int) -> bool:
    #     return self.set_inference(MaaInferenceExecutionProviderEnum.CUDA, nvidia_gpu_id)

    def set_gpu(self, gpu_id: int) -> bool:
        """
        Deprecated, please use `use_directml`, `use_coreml` or `use_cuda` instead.
        """
        if gpu_id < 0:
            return False
        return self.use_directml(gpu_id)

    def set_cpu(self) -> bool:
        """
        Deprecated, please use `use_cpu` instead.
        """
        return self.use_cpu()

    def set_auto_device(self) -> bool:
        """
        Deprecated, please use `use_auto_ep` instead.
        """
        return self.use_auto_ep()

    def custom_recognition(self, name: str):

        def wrapper_recognition(recognition):
            self.register_custom_recognition(name=name, recognition=recognition())
            return recognition

        return wrapper_recognition

    def register_custom_recognition(
        self, name: str, recognition: "CustomRecognition"  # type: ignore
    ) -> bool:

        # avoid gc
        self._custom_recognition_holder[name] = recognition

        return bool(
            Library.framework().MaaResourceRegisterCustomRecognition(
                self._handle,
                name.encode(),
                recognition.c_handle,
                recognition.c_arg,
            )
        )

    def unregister_custom_recognition(self, name: str) -> bool:
        self._custom_recognition_holder.pop(name, None)

        return bool(
            Library.framework().MaaResourceUnregisterCustomRecognition(
                self._handle,
                name.encode(),
            )
        )

    def clear_custom_recognition(self) -> bool:
        self._custom_recognition_holder.clear()

        return bool(
            Library.framework().MaaResourceClearCustomRecognition(
                self._handle,
            )
        )

    def custom_action(self, name: str):

        def wrapper_action(action):
            self.register_custom_action(name=name, action=action())
            return action

        return wrapper_action

    def register_custom_action(self, name: str, action: "CustomAction") -> bool:  # type: ignore
        # avoid gc
        self._custom_action_holder[name] = action

        return bool(
            Library.framework().MaaResourceRegisterCustomAction(
                self._handle,
                name.encode(),
                action.c_handle,
                action.c_arg,
            )
        )

    def unregister_custom_action(self, name: str) -> bool:
        self._custom_action_holder.pop(name, None)

        return bool(
            Library.framework().MaaResourceUnregisterCustomAction(
                self._handle,
                name.encode(),
            )
        )

    def clear_custom_action(self) -> bool:
        self._custom_action_holder.clear()

        return bool(
            Library.framework().MaaResourceClearCustomAction(
                self._handle,
            )
        )

    @property
    def node_list(self) -> list[str]:
        """
        Returns a list of node names.
        """
        buffer = StringListBuffer()
        if not Library.framework().MaaResourceGetNodeList(self._handle, buffer._handle):
            raise RuntimeError("Failed to get node list.")
        return buffer.get()

    @property
    def hash(self) -> str:
        buffer = StringBuffer()
        if not Library.framework().MaaResourceGetHash(self._handle, buffer._handle):
            raise RuntimeError("Failed to get hash.")
        return buffer.get()

    ### private ###

    def set_inference(self, execution_provider: int, device_id: int) -> bool:
        cep = ctypes.c_int32(execution_provider)
        cdevice = ctypes.c_int32(device_id)
        return bool(
            Library.framework().MaaResourceSetOption(
                self._handle,
                MaaResOptionEnum.InferenceExecutionProvider,
                ctypes.pointer(cep),
                ctypes.sizeof(ctypes.c_int32),
            )
        ) and bool(
            Library.framework().MaaResourceSetOption(
                self._handle,
                MaaResOptionEnum.InferenceDevice,
                ctypes.pointer(cdevice),
                ctypes.sizeof(ctypes.c_int32),
            )
        )

    def _status(self, id: int) -> ctypes.c_int32:
        return Library.framework().MaaResourceStatus(self._handle, id)

    def _wait(self, id: int) -> ctypes.c_int32:
        return Library.framework().MaaResourceWait(self._handle, id)

    _api_properties_initialized: bool = False

    @staticmethod
    def _set_api_properties():
        if Resource._api_properties_initialized:
            return
        Resource._api_properties_initialized = True

        Library.framework().MaaResourceCreate.restype = MaaResourceHandle
        Library.framework().MaaResourceCreate.argtypes = [
            MaaNotificationCallback,
            ctypes.c_void_p,
        ]

        Library.framework().MaaResourceDestroy.restype = None
        Library.framework().MaaResourceDestroy.argtypes = [MaaResourceHandle]

        Library.framework().MaaResourcePostBundle.restype = MaaResId
        Library.framework().MaaResourcePostBundle.argtypes = [
            MaaResourceHandle,
            ctypes.c_char_p,
        ]

        Library.framework().MaaResourceStatus.restype = MaaStatus
        Library.framework().MaaResourceStatus.argtypes = [
            MaaResourceHandle,
            MaaResId,
        ]

        Library.framework().MaaResourceWait.restype = MaaStatus
        Library.framework().MaaResourceWait.argtypes = [
            MaaResourceHandle,
            MaaResId,
        ]

        Library.framework().MaaResourceLoaded.restype = MaaBool
        Library.framework().MaaResourceLoaded.argtypes = [MaaResourceHandle]

        Library.framework().MaaResourceClear.restype = MaaBool
        Library.framework().MaaResourceClear.argtypes = [MaaResourceHandle]

        Library.framework().MaaResourceOverridePipeline.restype = MaaBool
        Library.framework().MaaResourceOverridePipeline.argtypes = [
            MaaResourceHandle,
            ctypes.c_char_p,
        ]

        Library.framework().MaaResourceOverrideNext.restype = MaaBool
        Library.framework().MaaResourceOverrideNext.argtypes = [
            MaaResourceHandle,
            ctypes.c_char_p,
            MaaStringListBufferHandle,
        ]

        Library.framework().MaaResourceGetNodeData.restype = MaaBool
        Library.framework().MaaResourceGetNodeData.argtypes = [
            MaaContextHandle,
            ctypes.c_char_p,
            MaaStringBufferHandle,
        ]

        Library.framework().MaaResourceGetHash.restype = MaaBool
        Library.framework().MaaResourceGetHash.argtypes = [
            MaaResourceHandle,
            MaaStringBufferHandle,
        ]

        Library.framework().MaaResourceSetOption.restype = MaaBool
        Library.framework().MaaResourceSetOption.argtypes = [
            MaaResourceHandle,
            MaaResOption,
            MaaOptionValue,
            MaaOptionValueSize,
        ]

        Library.framework().MaaResourceRegisterCustomRecognition.restype = MaaBool
        Library.framework().MaaResourceRegisterCustomRecognition.argtypes = [
            MaaResourceHandle,
            ctypes.c_char_p,
            MaaCustomRecognitionCallback,
            ctypes.c_void_p,
        ]

        Library.framework().MaaResourceUnregisterCustomRecognition.restype = MaaBool
        Library.framework().MaaResourceUnregisterCustomRecognition.argtypes = [
            MaaResourceHandle,
            ctypes.c_char_p,
        ]

        Library.framework().MaaResourceClearCustomRecognition.restype = MaaBool
        Library.framework().MaaResourceClearCustomRecognition.argtypes = [
            MaaResourceHandle,
        ]

        Library.framework().MaaResourceRegisterCustomAction.restype = MaaBool
        Library.framework().MaaResourceRegisterCustomAction.argtypes = [
            MaaResourceHandle,
            ctypes.c_char_p,
            MaaCustomActionCallback,
            ctypes.c_void_p,
        ]

        Library.framework().MaaResourceUnregisterCustomAction.restype = MaaBool
        Library.framework().MaaResourceUnregisterCustomAction.argtypes = [
            MaaResourceHandle,
            ctypes.c_char_p,
        ]

        Library.framework().MaaResourceClearCustomAction.restype = MaaBool
        Library.framework().MaaResourceClearCustomAction.argtypes = [
            MaaResourceHandle,
        ]

        Library.framework().MaaResourceGetNodeList.restype = MaaBool
        Library.framework().MaaResourceGetNodeList.argtypes = [
            MaaResourceHandle,
            MaaStringListBufferHandle,
        ]
