import ctypes
import pathlib
import json
from typing import Optional, Union, List, Dict
from dataclasses import dataclass, field

import numpy

from .event_sink import EventSink, NotificationType
from .define import *
from .job import Job
from .library import Library
from .buffer import StringBuffer, StringListBuffer, ImageBuffer
from .pipeline import JPipelineData, JPipelineParser


class Resource:
    _handle: MaaResourceHandle
    _own: bool

    ### public ###

    def __init__(
        self,
        notification_handler: None = None,
        handle: Optional[MaaResourceHandle] = None,
    ):
        """创建资源 / Create resource

        Args:
            notification_handler: 已废弃，请使用 add_sink 代替 / Deprecated, use add_sink instead
            handle: 可选的外部句柄 / Optional external handle

        Raises:
            NotImplementedError: 如果提供了 notification_handler
            RuntimeError: 如果创建失败
        """
        if notification_handler:
            raise NotImplementedError(
                "NotificationHandler is deprecated, use add_sink instead."
            )

        self._set_api_properties()

        if handle:
            self._handle = handle
            self._own = False
        else:
            self._handle = Library.framework().MaaResourceCreate()
            self._own = True

        if not self._handle:
            raise RuntimeError("Failed to create resource.")

        self._custom_action_holder = {}
        self._custom_recognition_holder = {}

    def __del__(self):
        if self._handle and self._own:
            Library.framework().MaaResourceDestroy(self._handle)

    def post_bundle(self, path: Union[pathlib.Path, str]) -> Job:
        """异步加载资源 / Asynchronously load resources from path

        这是一个异步操作，会立即返回一个 Job 对象
        This is an asynchronous operation that immediately returns a Job object

        Args:
            path: 资源路径 / Resource path

        Returns:
            Job: 作业对象，可通过 status/wait 查询状态 / Job object, can query status via status/wait
        """
        res_id = Library.framework().MaaResourcePostBundle(
            self._handle, str(path).encode()
        )
        return Job(res_id, self._status, self._wait)

    def override_pipeline(self, pipeline_override: Dict) -> bool:
        """覆盖 pipeline / Override pipeline_override

        Args:
            pipeline_override: 用于覆盖的 json / JSON for overriding

        Returns:
            bool: 是否成功 / Whether successful
        """
        pipeline_json = json.dumps(pipeline_override, ensure_ascii=False)

        return bool(
            Library.framework().MaaResourceOverridePipeline(
                self._handle,
                pipeline_json.encode(),
            )
        )

    def override_next(self, name: str, next_list: List[str]) -> bool:
        """覆盖任务的 next 列表 / Override the next list of task

        注意：此方法会直接设置 next 列表，即使节点不存在也会创建
        Note: This method directly sets the next list, creating the node if it doesn't exist

        Args:
            name: 任务名 / Task name
            next_list: next 列表 / Next list

        Returns:
            bool: 总是返回 True / Always returns True
        """
        list_buffer = StringListBuffer()
        list_buffer.set(next_list)

        return bool(
            Library.framework().MaaResourceOverrideNext(
                self._handle, name.encode(), list_buffer._handle
            )
        )

    def override_image(self, image_name: str, image: numpy.ndarray) -> bool:
        """覆盖图片 / Override the image corresponding to image_name

        Args:
            image_name: 图片名 / Image name
            image: 图片数据 / Image data

        Returns:
            bool: 总是返回 True / Always returns True
        """
        image_buffer = ImageBuffer()
        image_buffer.set(image)

        return bool(
            Library.framework().MaaResourceOverrideImage(
                self._handle, image_name.encode(), image_buffer._handle
            )
        )

    def get_node_data(self, name: str) -> Optional[Dict]:
        """获取任务当前的定义 / Get the current definition of task

        Args:
            name: 任务名 / Task name

        Returns:
            Optional[Dict]: 任务定义字典，如果不存在则返回 None / Task definition dict, or None if not exists
        """
        string_buffer = StringBuffer()
        if not Library.framework().MaaResourceGetNodeData(
            self._handle, name.encode(), string_buffer._handle
        ):
            return None
        data = string_buffer.get()
        if not data:
            return None

        try:
            return json.loads(data)
        except json.JSONDecodeError:
            return None

    def get_node_object(self, name: str) -> Optional[JPipelineData]:
        node_data = self.get_node_data(name)

        if not node_data:
            return None

        return JPipelineParser.parse_pipeline_data(node_data)

    @property
    def loaded(self) -> bool:
        """判断是否加载正常 / Check if resources loaded normally

        Returns:
            bool: 是否已加载 / Whether loaded
        """
        return bool(Library.framework().MaaResourceLoaded(self._handle))

    def clear(self) -> bool:
        """清除已加载内容 / Clear loaded content

        如果资源正在加载中，此方法会失败
        This method will fail if resources are currently loading

        Returns:
            bool: 成功返回 True，如果正在加载中则返回 False / Returns True on success, False if currently loading
        """
        return bool(Library.framework().MaaResourceClear(self._handle))

    def use_cpu(self) -> bool:
        """使用 CPU 进行推理 / Use CPU for inference

        Returns:
            bool: 是否成功 / Whether successful
        """
        return self.set_inference(
            MaaInferenceExecutionProviderEnum.CPU, MaaInferenceDeviceEnum.CPU
        )

    def use_directml(self, device_id: int = MaaInferenceDeviceEnum.Auto) -> bool:
        """使用 DirectML 进行推理 / Use DirectML for inference

        Args:
            device_id: 设备 id，默认为自动选择 / Device id, default is Auto

        Returns:
            bool: 是否成功 / Whether successful
        """
        return self.set_inference(MaaInferenceExecutionProviderEnum.DirectML, device_id)

    def use_coreml(self, coreml_flag: int = MaaInferenceDeviceEnum.Auto) -> bool:
        """使用 CoreML 进行推理 / Use CoreML for inference

        Args:
            coreml_flag: CoreML 标志，默认为自动选择 / CoreML flag, default is Auto

        Returns:
            bool: 是否成功 / Whether successful
        """
        return self.set_inference(MaaInferenceExecutionProviderEnum.CoreML, coreml_flag)

    def use_auto_ep(self) -> bool:
        """自动选择推理执行提供者 / Auto select inference execution provider

        Returns:
            bool: 是否成功 / Whether successful
        """
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
        """注册自定义识别器 / Register a custom recognizer

        Args:
            name: 名称 / Name
            recognition: 自定义识别器 / Custom recognizer

        Returns:
            bool: 是否成功 / Whether successful
        """
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
        """移除自定义识别器 / Remove the custom recognizer

        Args:
            name: 名称 / Name

        Returns:
            bool: 是否成功 / Whether successful
        """
        self._custom_recognition_holder.pop(name, None)

        return bool(
            Library.framework().MaaResourceUnregisterCustomRecognition(
                self._handle,
                name.encode(),
            )
        )

    def clear_custom_recognition(self) -> bool:
        """移除所有自定义识别器 / Remove all custom recognizers

        Returns:
            bool: 是否成功 / Whether successful
        """
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
        """注册自定义操作 / Register a custom action

        Args:
            name: 名称 / Name
            action: 自定义操作 / Custom action

        Returns:
            bool: 是否成功 / Whether successful
        """
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
        """移除自定义操作 / Remove the custom action

        Args:
            name: 名称 / Name

        Returns:
            bool: 是否成功 / Whether successful
        """
        self._custom_action_holder.pop(name, None)

        return bool(
            Library.framework().MaaResourceUnregisterCustomAction(
                self._handle,
                name.encode(),
            )
        )

    def clear_custom_action(self) -> bool:
        """移除所有自定义操作 / Remove all custom actions

        Returns:
            bool: 是否成功 / Whether successful
        """
        self._custom_action_holder.clear()

        return bool(
            Library.framework().MaaResourceClearCustomAction(
                self._handle,
            )
        )

    @property
    def node_list(self) -> list[str]:
        """获取任务列表 / Get task list

        Returns:
            list[str]: 任务名列表 / List of task names

        Raises:
            RuntimeError: 如果获取失败
        """
        buffer = StringListBuffer()
        if not Library.framework().MaaResourceGetNodeList(self._handle, buffer._handle):
            raise RuntimeError("Failed to get node list.")
        return buffer.get()

    @property
    def hash(self) -> str:
        """获取资源 hash / Get resource hash

        Returns:
            str: 资源 hash / Resource hash

        Raises:
            RuntimeError: 如果获取失败
        """
        buffer = StringBuffer()
        if not Library.framework().MaaResourceGetHash(self._handle, buffer._handle):
            raise RuntimeError("Failed to get hash.")
        return buffer.get()

    _sink_holder: Dict[int, "ResourceEventSink"] = {}

    def add_sink(self, sink: "ResourceEventSink") -> Optional[int]:
        """添加资源事件监听器 / Add resource event listener

        Args:
            sink: 事件监听器 / Event sink

        Returns:
            Optional[int]: 监听器 id，失败返回 None / Listener id, or None if failed
        """
        sink_id = int(
            Library.framework().MaaResourceAddSink(
                self._handle, *EventSink._gen_c_param(sink)
            )
        )
        if sink_id == MaaInvalidId:
            return None

        self._sink_holder[sink_id] = sink
        return sink_id

    def remove_sink(self, sink_id: int) -> None:
        """移除资源事件监听器 / Remove resource event listener

        Args:
            sink_id: 监听器 id / Listener id
        """
        Library.framework().MaaResourceRemoveSink(self._handle, sink_id)
        self._sink_holder.pop(sink_id)

    def clear_sinks(self) -> None:
        """清除所有资源事件监听器 / Clear all resource event listeners"""
        Library.framework().MaaResourceClearSinks(self._handle)

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
        Library.framework().MaaResourceCreate.argtypes = []

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

        Library.framework().MaaResourceOverrideImage.restype = MaaBool
        Library.framework().MaaResourceOverrideImage.argtypes = [
            MaaResourceHandle,
            ctypes.c_char_p,
            MaaImageBufferHandle,
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

        Library.framework().MaaResourceAddSink.restype = MaaSinkId
        Library.framework().MaaResourceAddSink.argtypes = [
            MaaResourceHandle,
            MaaEventCallback,
            ctypes.c_void_p,
        ]

        Library.framework().MaaResourceRemoveSink.restype = None
        Library.framework().MaaResourceRemoveSink.argtypes = [
            MaaResourceHandle,
            MaaSinkId,
        ]

        Library.framework().MaaResourceClearSinks.restype = None
        Library.framework().MaaResourceClearSinks.argtypes = [MaaResourceHandle]


class ResourceEventSink(EventSink):

    @dataclass
    class ResourceLoadingDetail:
        res_id: int
        hash: str
        path: str

    def on_resource_loading(
        self,
        resource: Resource,
        noti_type: NotificationType,
        detail: ResourceLoadingDetail,
    ):
        pass

    def on_raw_notification(self, resource: Resource, msg: str, details: dict):
        pass

    def _on_raw_notification(self, handle: ctypes.c_void_p, msg: str, details: dict):

        resource = Resource(handle=handle)
        self.on_raw_notification(resource, msg, details)

        noti_type = EventSink._notification_type(msg)
        if msg.startswith("Resource.Loading"):
            detail = self.ResourceLoadingDetail(
                res_id=details["res_id"],
                hash=details["hash"],
                path=details["path"],
            )
            self.on_resource_loading(resource, noti_type, detail)

        else:
            self.on_unknown_notification(resource, msg, details)
