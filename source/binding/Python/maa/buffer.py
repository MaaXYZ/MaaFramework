import copy
import ctypes
from typing import List, Optional, Union

import numpy

from .define import *
from .library import Library


class StringBuffer:
    """字符串缓冲区 / String buffer

    用于在 Python 和 C API 之间传递字符串数据。
    Used to pass string data between Python and C API.
    """

    _handle: MaaStringBufferHandle
    _own: bool

    def __init__(self, handle: Optional[MaaStringBufferHandle] = None):
        self._set_api_properties()

        if handle:
            self._handle = handle
            self._own = False
        else:
            self._handle = Library.framework().MaaStringBufferCreate()
            self._own = True

        if not self._handle:
            raise RuntimeError("Failed to create string buffer.")

    def __del__(self):
        if self._handle and self._own:
            Library.framework().MaaStringBufferDestroy(self._handle)

    def get(self) -> str:
        """获取缓冲区内容 / Get buffer content

        Returns:
            str: 字符串内容 / String content
        """
        buff = Library.framework().MaaStringBufferGet(self._handle)
        sz = Library.framework().MaaStringBufferSize(self._handle)
        return ctypes.string_at(buff, sz).decode()

    def set(self, value: Union[str, bytes]) -> bool:
        """设置缓冲区内容 / Set buffer content

        Args:
            value: 字符串或字节数据 / String or bytes data

        Returns:
            bool: 是否成功 / Whether successful
        """
        if isinstance(value, str):
            value = value.encode()
        return bool(
            Library.framework().MaaStringBufferSetEx(self._handle, value, len(value))
        )

    @property
    def empty(self) -> bool:
        """判断缓冲区是否为空 / Check if buffer is empty

        Returns:
            bool: 是否为空 / Whether empty
        """
        return bool(Library.framework().MaaStringBufferIsEmpty(self._handle))

    def clear(self) -> bool:
        """清空缓冲区 / Clear buffer

        Returns:
            bool: 是否成功 / Whether successful
        """
        return bool(Library.framework().MaaStringBufferClear(self._handle))

    _api_properties_initialized: bool = False

    @staticmethod
    def _set_api_properties():
        if StringBuffer._api_properties_initialized:
            return
        StringBuffer._api_properties_initialized = True

        Library.framework().MaaStringBufferCreate.restype = MaaStringBufferHandle
        Library.framework().MaaStringBufferCreate.argtypes = []

        Library.framework().MaaStringBufferDestroy.restype = None
        Library.framework().MaaStringBufferDestroy.argtypes = [MaaStringBufferHandle]

        Library.framework().MaaStringBufferIsEmpty.restype = MaaBool
        Library.framework().MaaStringBufferIsEmpty.argtypes = [MaaStringBufferHandle]

        Library.framework().MaaStringBufferClear.restype = MaaBool
        Library.framework().MaaStringBufferClear.argtypes = [MaaStringBufferHandle]

        Library.framework().MaaStringBufferGet.restype = ctypes.c_char_p
        Library.framework().MaaStringBufferGet.argtypes = [MaaStringBufferHandle]

        Library.framework().MaaStringBufferSize.restype = MaaSize
        Library.framework().MaaStringBufferSize.argtypes = [MaaStringBufferHandle]

        Library.framework().MaaStringBufferSet.restype = MaaBool
        Library.framework().MaaStringBufferSet.argtypes = [
            MaaStringBufferHandle,
            ctypes.c_char_p,
        ]

        Library.framework().MaaStringBufferSetEx.restype = MaaBool
        Library.framework().MaaStringBufferSetEx.argtypes = [
            MaaStringBufferHandle,
            ctypes.c_char_p,
            MaaSize,
        ]


class StringListBuffer:
    """字符串列表缓冲区 / String list buffer

    用于在 Python 和 C API 之间传递字符串列表数据。
    Used to pass string list data between Python and C API.
    """

    _handle: MaaStringListBufferHandle
    _own: bool

    def __init__(self, handle: Optional[MaaStringListBufferHandle] = None):
        self._set_api_properties()

        if handle:
            self._handle = handle
            self._own = False
        else:
            self._handle = Library.framework().MaaStringListBufferCreate()
            self._own = True

        if not self._handle:
            raise RuntimeError("Failed to create string list buffer.")

    def __del__(self):
        if self._handle and self._own:
            Library.framework().MaaStringListBufferDestroy(self._handle)

    def get(self) -> List[str]:
        """获取字符串列表 / Get string list

        Returns:
            List[str]: 字符串列表 / String list
        """
        count = Library.framework().MaaStringListBufferSize(self._handle)
        result = []
        for i in range(count):
            buff = Library.framework().MaaStringListBufferAt(self._handle, i)
            s = StringBuffer(buff).get()
            result.append(s)
        return result

    def set(self, value: List[str]) -> bool:
        """设置字符串列表 / Set string list

        Args:
            value: 字符串列表 / String list

        Returns:
            bool: 是否成功 / Whether successful
        """
        self.clear()
        for s in value:
            if not self.append(s):
                return False
        return True

    def append(self, value: str) -> bool:
        """追加字符串 / Append string

        Args:
            value: 要追加的字符串 / String to append

        Returns:
            bool: 是否成功 / Whether successful
        """
        buff = StringBuffer()
        buff.set(value)
        return bool(
            Library.framework().MaaStringListBufferAppend(self._handle, buff._handle)
        )

    def remove(self, index: int) -> bool:
        """移除指定索引的字符串 / Remove string at index

        Args:
            index: 要移除的索引 / Index to remove

        Returns:
            bool: 是否成功 / Whether successful
        """
        return bool(Library.framework().MaaStringListBufferRemove(self._handle, index))

    def clear(self) -> bool:
        """清空列表 / Clear list

        Returns:
            bool: 是否成功 / Whether successful
        """
        return bool(Library.framework().MaaStringListBufferClear(self._handle))

    _api_properties_initialized: bool = False

    @staticmethod
    def _set_api_properties():
        if StringListBuffer._api_properties_initialized:
            return
        StringListBuffer._api_properties_initialized = True

        Library.framework().MaaStringListBufferCreate.restype = (
            MaaStringListBufferHandle
        )
        Library.framework().MaaStringListBufferCreate.argtypes = []

        Library.framework().MaaStringListBufferDestroy.restype = None
        Library.framework().MaaStringListBufferDestroy.argtypes = [
            MaaStringListBufferHandle
        ]

        Library.framework().MaaStringListBufferIsEmpty.restype = MaaBool
        Library.framework().MaaStringListBufferIsEmpty.argtypes = [
            MaaStringListBufferHandle
        ]

        Library.framework().MaaStringListBufferClear.restype = MaaBool
        Library.framework().MaaStringListBufferClear.argtypes = [
            MaaStringListBufferHandle
        ]

        Library.framework().MaaStringListBufferSize.restype = MaaSize
        Library.framework().MaaStringListBufferSize.argtypes = [
            MaaStringListBufferHandle
        ]

        Library.framework().MaaStringListBufferAt.restype = MaaStringBufferHandle
        Library.framework().MaaStringListBufferAt.argtypes = [
            MaaStringListBufferHandle,
            MaaSize,
        ]

        Library.framework().MaaStringListBufferAppend.restype = MaaBool
        Library.framework().MaaStringListBufferAppend.argtypes = [
            MaaStringListBufferHandle,
            MaaStringBufferHandle,
        ]

        Library.framework().MaaStringListBufferRemove.restype = MaaBool
        Library.framework().MaaStringListBufferRemove.argtypes = [
            MaaStringListBufferHandle,
            MaaSize,
        ]


class ImageBuffer:
    """图像缓冲区 / Image buffer

    用于在 Python 和 C API 之间传递图像数据。图像格式为 BGR，与 OpenCV 兼容。
    Used to pass image data between Python and C API. Image format is BGR, compatible with OpenCV.
    """

    _handle: MaaImageBufferHandle
    _own: bool

    def __init__(self, c_handle: Optional[MaaImageBufferHandle] = None):
        self._set_api_properties()

        if c_handle:
            self._handle = c_handle
            self._own = False
        else:
            self._handle = Library.framework().MaaImageBufferCreate()
            self._own = True

        if not self._handle:
            raise RuntimeError("Failed to create string buffer.")

    def __del__(self):
        if self._handle and self._own:
            Library.framework().MaaImageBufferDestroy(self._handle)

    def get(self) -> numpy.ndarray:
        """获取图像数据 / Get image data

        Returns:
            numpy.ndarray: BGR 格式图像，形状为 (height, width, channels) / BGR format image with shape (height, width, channels)
        """
        buff = Library.framework().MaaImageBufferGetRawData(self._handle)
        if not buff:
            return numpy.ndarray((0, 0, 3), dtype=numpy.uint8)

        w = Library.framework().MaaImageBufferWidth(self._handle)
        h = Library.framework().MaaImageBufferHeight(self._handle)
        c = Library.framework().MaaImageBufferChannels(self._handle)
        return copy.deepcopy(
            numpy.ctypeslib.as_array(
                ctypes.cast(buff, ctypes.POINTER(ctypes.c_uint8)), shape=(h, w, c)
            )
        )

    def set(self, value: numpy.ndarray) -> bool:
        """设置图像数据 / Set image data

        Args:
            value: BGR 格式图像，形状为 (height, width, channels) / BGR format image with shape (height, width, channels)

        Returns:
            bool: 是否成功 / Whether successful

        Raises:
            TypeError: 如果 value 不是 numpy.ndarray
        """
        if not isinstance(value, numpy.ndarray):
            raise TypeError("value must be a numpy.ndarray")

        # 确保数组是 C-contiguous 的，避免切片视图导致的内存不连续问题
        if not value.flags['C_CONTIGUOUS']:
            value = numpy.ascontiguousarray(value)

        return bool(
            Library.framework().MaaImageBufferSetRawData(
                self._handle,
                value.ctypes.data,
                value.shape[1],
                value.shape[0],
                16,  # CV_8UC3
            )
        )

    @property
    def empty(self) -> bool:
        """判断缓冲区是否为空 / Check if buffer is empty

        Returns:
            bool: 是否为空 / Whether empty
        """
        return bool(Library.framework().MaaImageBufferIsEmpty(self._handle))

    def clear(self) -> bool:
        """清空缓冲区 / Clear buffer

        Returns:
            bool: 是否成功 / Whether successful
        """
        return bool(Library.framework().MaaImageBufferClear(self._handle))

    _api_properties_initialized: bool = False

    @staticmethod
    def _set_api_properties():
        if ImageBuffer._api_properties_initialized:
            return
        ImageBuffer._api_properties_initialized = True

        Library.framework().MaaImageBufferCreate.restype = MaaImageBufferHandle
        Library.framework().MaaImageBufferCreate.argtypes = []

        Library.framework().MaaImageBufferDestroy.restype = None
        Library.framework().MaaImageBufferDestroy.argtypes = [MaaImageBufferHandle]

        Library.framework().MaaImageBufferGetRawData.restype = ctypes.c_void_p
        Library.framework().MaaImageBufferGetRawData.argtypes = [MaaImageBufferHandle]

        Library.framework().MaaImageBufferWidth.restype = ctypes.c_int32
        Library.framework().MaaImageBufferWidth.argtypes = [MaaImageBufferHandle]

        Library.framework().MaaImageBufferHeight.restype = ctypes.c_int32
        Library.framework().MaaImageBufferHeight.argtypes = [MaaImageBufferHandle]

        Library.framework().MaaImageBufferChannels.restype = ctypes.c_int32
        Library.framework().MaaImageBufferChannels.argtypes = [MaaImageBufferHandle]

        Library.framework().MaaImageBufferType.restype = ctypes.c_int32
        Library.framework().MaaImageBufferType.argtypes = [MaaImageBufferHandle]

        Library.framework().MaaImageBufferSetRawData.restype = MaaBool
        Library.framework().MaaImageBufferSetRawData.argtypes = [
            MaaImageBufferHandle,
            ctypes.c_void_p,
            ctypes.c_int32,
            ctypes.c_int32,
            ctypes.c_int32,
        ]

        Library.framework().MaaImageBufferIsEmpty.restype = MaaBool
        Library.framework().MaaImageBufferIsEmpty.argtypes = [MaaImageBufferHandle]

        Library.framework().MaaImageBufferClear.restype = MaaBool
        Library.framework().MaaImageBufferClear.argtypes = [MaaImageBufferHandle]


class ImageListBuffer:
    """图像列表缓冲区 / Image list buffer

    用于在 Python 和 C API 之间传递图像列表数据。
    Used to pass image list data between Python and C API.
    """

    _handle: MaaImageListBufferHandle
    _own: bool

    def __init__(self, c_handle: Optional[MaaImageListBufferHandle] = None):
        self._set_api_properties()

        if c_handle:
            self._handle = c_handle
            self._own = False
        else:
            self._handle = Library.framework().MaaImageListBufferCreate()
            self._own = True

        if not self._handle:
            raise RuntimeError("Failed to create image list buffer.")

    def __del__(self):
        if self._handle and self._own:
            Library.framework().MaaImageListBufferDestroy(self._handle)

    def get(self) -> List[numpy.ndarray]:
        """获取图像列表 / Get image list

        Returns:
            List[numpy.ndarray]: 图像列表 / Image list
        """
        count = Library.framework().MaaImageListBufferSize(self._handle)
        result = []
        for i in range(count):
            buff = Library.framework().MaaImageListBufferAt(self._handle, i)
            img = ImageBuffer(buff).get()
            result.append(img)
        return result

    def set(self, value: List[numpy.ndarray]) -> bool:
        """设置图像列表 / Set image list

        Args:
            value: 图像列表 / Image list

        Returns:
            bool: 是否成功 / Whether successful
        """
        self.clear()
        for img in value:
            if not self.append(img):
                return False
        return True

    def append(self, value: numpy.ndarray) -> bool:
        """追加图像 / Append image

        Args:
            value: 要追加的图像 / Image to append

        Returns:
            bool: 是否成功 / Whether successful
        """
        buff = ImageBuffer()
        buff.set(value)
        return bool(
            Library.framework().MaaImageListBufferAppend(self._handle, buff._handle)
        )

    def remove(self, index: int) -> bool:
        """移除指定索引的图像 / Remove image at index

        Args:
            index: 要移除的索引 / Index to remove

        Returns:
            bool: 是否成功 / Whether successful
        """
        return bool(Library.framework().MaaImageListBufferRemove(self._handle, index))

    def clear(self) -> bool:
        """清空列表 / Clear list

        Returns:
            bool: 是否成功 / Whether successful
        """
        return bool(Library.framework().MaaImageListBufferClear(self._handle))

    _api_properties_initialized: bool = False

    @staticmethod
    def _set_api_properties():
        if ImageListBuffer._api_properties_initialized:
            return
        ImageListBuffer._api_properties_initialized = True

        Library.framework().MaaImageListBufferCreate.restype = MaaImageListBufferHandle
        Library.framework().MaaImageListBufferCreate.argtypes = []

        Library.framework().MaaImageListBufferDestroy.restype = None
        Library.framework().MaaImageListBufferDestroy.argtypes = [
            MaaImageListBufferHandle
        ]

        Library.framework().MaaImageListBufferIsEmpty.restype = MaaBool
        Library.framework().MaaImageListBufferIsEmpty.argtypes = [
            MaaImageListBufferHandle
        ]

        Library.framework().MaaImageListBufferClear.restype = MaaBool
        Library.framework().MaaImageListBufferClear.argtypes = [
            MaaImageListBufferHandle
        ]

        Library.framework().MaaImageListBufferSize.restype = MaaSize
        Library.framework().MaaImageListBufferSize.argtypes = [MaaImageListBufferHandle]

        Library.framework().MaaImageListBufferAt.restype = MaaImageBufferHandle
        Library.framework().MaaImageListBufferAt.argtypes = [
            MaaImageListBufferHandle,
            MaaSize,
        ]

        Library.framework().MaaImageListBufferAppend.restype = MaaBool
        Library.framework().MaaImageListBufferAppend.argtypes = [
            MaaImageListBufferHandle,
            MaaImageBufferHandle,
        ]

        Library.framework().MaaImageListBufferRemove.restype = MaaBool
        Library.framework().MaaImageListBufferRemove.argtypes = [
            MaaImageListBufferHandle,
            MaaSize,
        ]


class RectBuffer:
    """矩形缓冲区 / Rectangle buffer

    用于在 Python 和 C API 之间传递矩形数据（x, y, width, height）。
    Used to pass rectangle data (x, y, width, height) between Python and C API.
    """

    _handle: MaaRectHandle
    _own: bool

    def __init__(self, c_handle: Optional[MaaRectHandle] = None):
        self._set_api_properties()

        if c_handle:
            self._handle = c_handle
            self._own = False
        else:
            self._handle = Library.framework().MaaRectCreate()
            self._own = True

        if not self._handle:
            raise RuntimeError("Failed to create rect buffer.")

    def __del__(self):
        if self._handle and self._own:
            Library.framework().MaaRectDestroy(self._handle)

    def get(self) -> Rect:
        """获取矩形数据 / Get rectangle data

        Returns:
            Rect: 矩形对象 (x, y, width, height) / Rectangle object (x, y, width, height)
        """
        x = Library.framework().MaaRectGetX(self._handle)
        y = Library.framework().MaaRectGetY(self._handle)
        w = Library.framework().MaaRectGetW(self._handle)
        h = Library.framework().MaaRectGetH(self._handle)

        return Rect(x, y, w, h)

    def set(self, value: RectType) -> bool:
        """设置矩形数据 / Set rectangle data

        Args:
            value: 矩形数据，可以是 Rect、tuple、list 或 numpy.ndarray / Rectangle data, can be Rect, tuple, list, or numpy.ndarray

        Returns:
            bool: 是否成功 / Whether successful

        Raises:
            ValueError: 如果数据格式不正确
            TypeError: 如果类型不支持
        """
        if isinstance(value, numpy.ndarray):
            if value.ndim != 1:
                raise ValueError("value must be a 1D array")
            if value.shape[0] != 4:
                raise ValueError("value must have 4 elements")
            if value.dtype != numpy.int32:
                raise ValueError("value must be of type numpy.int32")
        elif isinstance(value, tuple) or isinstance(value, list):
            if len(value) != 4:
                raise ValueError("value must have 4 elements")
            value = numpy.array(value, dtype=numpy.int32)
        elif isinstance(value, Rect):
            pass
        else:
            raise TypeError("value must be a Rect, numpy.ndarray, tuple or list")

        return bool(
            Library.framework().MaaRectSet(
                self._handle, value[0], value[1], value[2], value[3]
            )
        )

    _api_properties_initialized: bool = False

    @staticmethod
    def _set_api_properties():
        if RectBuffer._api_properties_initialized:
            return
        RectBuffer._api_properties_initialized = True

        Library.framework().MaaRectCreate.restype = MaaRectHandle
        Library.framework().MaaRectCreate.argtypes = []

        Library.framework().MaaRectDestroy.restype = None
        Library.framework().MaaRectDestroy.argtypes = [MaaRectHandle]

        Library.framework().MaaRectGetX.restype = ctypes.c_int32
        Library.framework().MaaRectGetX.argtypes = [MaaRectHandle]

        Library.framework().MaaRectGetY.restype = ctypes.c_int32
        Library.framework().MaaRectGetY.argtypes = [MaaRectHandle]

        Library.framework().MaaRectGetW.restype = ctypes.c_int32
        Library.framework().MaaRectGetW.argtypes = [MaaRectHandle]

        Library.framework().MaaRectGetH.restype = ctypes.c_int32
        Library.framework().MaaRectGetH.argtypes = [MaaRectHandle]

        Library.framework().MaaRectSet.restype = MaaBool
        Library.framework().MaaRectSet.argtypes = [
            MaaRectHandle,
            ctypes.c_int32,
            ctypes.c_int32,
            ctypes.c_int32,
            ctypes.c_int32,
        ]
