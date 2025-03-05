import copy
import ctypes
from typing import List, Optional, Union

import numpy

from .define import *
from .library import Library


class StringBuffer:
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
        buff = Library.framework().MaaStringBufferGet(self._handle)
        sz = Library.framework().MaaStringBufferSize(self._handle)
        return ctypes.string_at(buff, sz).decode()

    def set(self, value: Union[str, bytes]) -> bool:
        if isinstance(value, str):
            value = value.encode()
        return bool(
            Library.framework().MaaStringBufferSetEx(self._handle, value, len(value))
        )

    @property
    def empty(self) -> bool:
        return bool(Library.framework().MaaStringBufferIsEmpty(self._handle))

    def clear(self) -> bool:
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
        count = Library.framework().MaaStringListBufferSize(self._handle)
        result = []
        for i in range(count):
            buff = Library.framework().MaaStringListBufferAt(self._handle, i)
            s = StringBuffer(buff).get()
            result.append(s)
        return result

    def set(self, value: List[str]) -> bool:
        self.clear()
        for s in value:
            if not self.append(s):
                return False
        return True

    def append(self, value: str) -> bool:
        buff = StringBuffer()
        buff.set(value)
        return bool(
            Library.framework().MaaStringListBufferAppend(self._handle, buff._handle)
        )

    def remove(self, index: int) -> bool:
        return bool(Library.framework().MaaStringListBufferRemove(self._handle, index))

    def clear(self) -> bool:
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
        if not isinstance(value, numpy.ndarray):
            raise TypeError("value must be a numpy.ndarray")

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
        return bool(Library.framework().MaaImageBufferIsEmpty(self._handle))

    def clear(self) -> bool:
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
        count = Library.framework().MaaImageListBufferSize(self._handle)
        result = []
        for i in range(count):
            buff = Library.framework().MaaImageListBufferAt(self._handle, i)
            img = ImageBuffer(buff).get()
            result.append(img)
        return result

    def set(self, value: List[numpy.ndarray]) -> bool:
        self.clear()
        for img in value:
            if not self.append(img):
                return False
        return True

    def append(self, value: numpy.ndarray) -> bool:
        buff = ImageBuffer()
        buff.set(value)
        return bool(
            Library.framework().MaaImageListBufferAppend(self._handle, buff._handle)
        )

    def remove(self, index: int) -> bool:
        return bool(Library.framework().MaaImageListBufferRemove(self._handle, index))

    def clear(self) -> bool:
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
        x = Library.framework().MaaRectGetX(self._handle)
        y = Library.framework().MaaRectGetY(self._handle)
        w = Library.framework().MaaRectGetW(self._handle)
        h = Library.framework().MaaRectGetH(self._handle)

        return Rect(x, y, w, h)

    def set(self, value: RectType) -> bool:
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
