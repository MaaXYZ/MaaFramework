import copy
import ctypes
from typing import List, Optional, Tuple, Union

import numpy
from PIL import Image

from .define import *
from .library import Library


class StringBuffer:
    _handle: MaaStringBufferHandle
    _own: bool

    @property
    def c_handle(self) -> MaaStringBufferHandle:
        return self._handle

    def __init__(self, c_handle: Optional[MaaStringBufferHandle] = None):
        if not Library.initialized:
            raise RuntimeError(
                "Library not initialized, please call `library.open()` first."
            )
        self._set_api_properties()

        if c_handle:
            self._handle = c_handle
            self._own = False
        else:
            self._handle = Library.framework.MaaCreateStringBuffer()
            self._own = True

        if not self._handle:
            raise RuntimeError("Failed to create string buffer.")

    def __del__(self):
        if self._handle and self._own:
            Library.framework.MaaDestroyStringBuffer(self._handle)

    def get(self) -> str:
        buff = Library.framework.MaaGetString(self._handle)
        sz = Library.framework.MaaGetStringSize(self._handle)
        return ctypes.string_at(buff, sz).decode()

    def set(self, value: Union[str, bytes]) -> bool:
        if isinstance(value, str):
            value = value.encode()
        return bool(Library.framework.MaaSetString(self._handle, value))

    @property
    def empty(self) -> bool:
        return bool(Library.framework.MaaIsStringEmpty(self._handle))

    def clear(self) -> bool:
        return bool(Library.framework.MaaClearString(self._handle))

    _api_properties_initialized: bool = False

    @staticmethod
    def _set_api_properties():
        if StringBuffer._api_properties_initialized:
            return
        StringBuffer._api_properties_initialized = True

        Library.framework.MaaCreateStringBuffer.restype = MaaStringBufferHandle
        Library.framework.MaaCreateStringBuffer.argtypes = None

        Library.framework.MaaDestroyStringBuffer.restype = None
        Library.framework.MaaDestroyStringBuffer.argtypes = [MaaStringBufferHandle]

        Library.framework.MaaIsStringEmpty.restype = MaaBool
        Library.framework.MaaIsStringEmpty.argtypes = [MaaStringBufferHandle]

        Library.framework.MaaClearString.restype = MaaBool
        Library.framework.MaaClearString.argtypes = [MaaStringBufferHandle]

        Library.framework.MaaGetString.restype = MaaStringView
        Library.framework.MaaGetString.argtypes = [MaaStringBufferHandle]

        Library.framework.MaaGetStringSize.restype = MaaSize
        Library.framework.MaaGetStringSize.argtypes = [MaaStringBufferHandle]

        Library.framework.MaaSetString.restype = MaaBool
        Library.framework.MaaSetString.argtypes = [MaaStringBufferHandle, MaaStringView]

        Library.framework.MaaSetStringEx.restype = MaaBool
        Library.framework.MaaSetStringEx.argtypes = [
            MaaStringBufferHandle,
            MaaStringView,
            MaaSize,
        ]


class StringListBuffer:
    _handle: MaaStringListBufferHandle
    _own: bool

    @property
    def c_handle(self) -> MaaStringListBufferHandle:
        return self._handle

    def __init__(self, c_handle: Optional[MaaStringListBufferHandle] = None):
        if not Library.initialized:
            raise RuntimeError(
                "Library not initialized, please call `library.open()` first."
            )
        self._set_api_properties()

        if c_handle:
            self._handle = c_handle
            self._own = False
        else:
            self._handle = Library.framework.MaaCreateStringListBuffer()
            self._own = True

        if not self._handle:
            raise RuntimeError("Failed to create string list buffer.")

    def __del__(self):
        if self._handle and self._own:
            Library.framework.MaaDestroyStringListBuffer(self._handle)

    def get(self) -> List[str]:
        count = Library.framework.MaaGetStringListSize(self._handle)
        result = []
        for i in range(count):
            buff = Library.framework.MaaGetStringListAt(self._handle, i)
            s = StringBuffer(buff).get()
            result.append(s)
        return result

    def set(self, value: List[str]) -> bool:
        Library.framework.MaaClearStringList(self._handle)
        for s in value:
            buff = StringBuffer()
            buff.set(s)
            if not Library.framework.MaaStringListAppend(self._handle, buff.c_handle):
                return False
        return True

    def append(self, value: str) -> bool:
        buff = StringBuffer()
        buff.set(value)
        return bool(Library.framework.MaaStringListAppend(self._handle, buff.c_handle))

    def remove(self, index: int) -> bool:
        return bool(Library.framework.MaaStringListRemove(self._handle, index))

    def clear(self) -> bool:
        return bool(Library.framework.MaaClearStringList(self._handle))

    _api_properties_initialized: bool = False

    @staticmethod
    def _set_api_properties():
        if StringListBuffer._api_properties_initialized:
            return
        StringListBuffer._api_properties_initialized = True

        Library.framework.MaaCreateStringListBuffer.restype = MaaStringListBufferHandle
        Library.framework.MaaCreateStringListBuffer.argtypes = None

        Library.framework.MaaDestroyStringListBuffer.restype = None
        Library.framework.MaaDestroyStringListBuffer.argtypes = [
            MaaStringListBufferHandle
        ]

        Library.framework.MaaIsStringListEmpty.restype = MaaBool
        Library.framework.MaaIsStringListEmpty.argtypes = [MaaStringListBufferHandle]

        Library.framework.MaaClearStringList.restype = MaaBool
        Library.framework.MaaClearStringList.argtypes = [MaaStringListBufferHandle]

        Library.framework.MaaGetStringListSize.restype = MaaSize
        Library.framework.MaaGetStringListSize.argtypes = [MaaStringListBufferHandle]

        Library.framework.MaaGetStringListAt.restype = MaaStringBufferHandle
        Library.framework.MaaGetStringListAt.argtypes = [
            MaaStringListBufferHandle,
            MaaSize,
        ]

        Library.framework.MaaStringListAppend.restype = MaaBool
        Library.framework.MaaStringListAppend.argtypes = [
            MaaStringListBufferHandle,
            MaaStringBufferHandle,
        ]

        Library.framework.MaaStringListRemove.restype = MaaBool
        Library.framework.MaaStringListRemove.argtypes = [
            MaaStringListBufferHandle,
            MaaSize,
        ]


class ImageBuffer:
    _handle: MaaImageBufferHandle
    _own: bool

    @property
    def c_handle(self) -> MaaImageBufferHandle:
        return self._handle

    def __init__(self, c_handle: Optional[MaaImageBufferHandle] = None):
        if not Library.initialized:
            raise RuntimeError(
                "Library not initialized, please call `library.open()` first."
            )

        self._set_api_properties()

        if c_handle:
            self._handle = c_handle
            self._own = False
        else:
            self._handle = Library.framework.MaaCreateImageBuffer()
            self._own = True

        if not self._handle:
            raise RuntimeError("Failed to create string buffer.")

    def __del__(self):
        if self._handle and self._own:
            Library.framework.MaaDestroyImageBuffer(self._handle)

    def get(self) -> numpy.ndarray:
        buff = Library.framework.MaaGetImageRawData(self._handle)
        if not buff:
            return numpy.ndarray((0, 0, 3), dtype=numpy.uint8)

        w = Library.framework.MaaGetImageWidth(self._handle)
        h = Library.framework.MaaGetImageHeight(self._handle)
        # t = Library.framework.MaaGetImageType(self._handle)
        return copy.deepcopy(
            numpy.ctypeslib.as_array(
                ctypes.cast(buff, ctypes.POINTER(ctypes.c_uint8)), shape=(h, w, 3)
            )
        )

    def set(self, value: Union[numpy.ndarray, Image.Image]) -> bool:
        if isinstance(value, Image.Image):
            value = numpy.array(value)

        if not isinstance(value, numpy.ndarray):
            raise TypeError("value must be a numpy.ndarray or PIL.Image")

        return bool(
            Library.framework.MaaSetImageRawData(
                self._handle,
                value.ctypes.data,
                value.shape[1],
                value.shape[0],
                16,  # CV_8UC3
            )
        )

    @property
    def empty(self) -> bool:
        return bool(Library.framework.MaaIsImageEmpty(self._handle))

    def clear(self) -> bool:
        return bool(Library.framework.MaaClearImage(self._handle))

    _api_properties_initialized: bool = False

    @staticmethod
    def _set_api_properties():
        if ImageBuffer._api_properties_initialized:
            return
        ImageBuffer._api_properties_initialized = True

        Library.framework.MaaCreateImageBuffer.restype = MaaImageBufferHandle
        Library.framework.MaaCreateImageBuffer.argtypes = []

        Library.framework.MaaDestroyImageBuffer.restype = None
        Library.framework.MaaDestroyImageBuffer.argtypes = [MaaImageBufferHandle]

        Library.framework.MaaGetImageRawData.restype = MaaImageRawData
        Library.framework.MaaGetImageRawData.argtypes = [MaaImageBufferHandle]

        Library.framework.MaaGetImageWidth.restype = ctypes.c_int32
        Library.framework.MaaGetImageWidth.argtypes = [MaaImageBufferHandle]

        Library.framework.MaaGetImageHeight.restype = ctypes.c_int32
        Library.framework.MaaGetImageHeight.argtypes = [MaaImageBufferHandle]

        Library.framework.MaaGetImageType.restype = ctypes.c_int32
        Library.framework.MaaGetImageType.argtypes = [MaaImageBufferHandle]

        Library.framework.MaaSetImageRawData.restype = MaaBool
        Library.framework.MaaSetImageRawData.argtypes = [
            MaaImageBufferHandle,
            MaaImageRawData,
            ctypes.c_int32,
            ctypes.c_int32,
            ctypes.c_int32,
        ]

        Library.framework.MaaIsImageEmpty.restype = MaaBool
        Library.framework.MaaIsImageEmpty.argtypes = [MaaImageBufferHandle]

        Library.framework.MaaClearImage.restype = MaaBool
        Library.framework.MaaClearImage.argtypes = [MaaImageBufferHandle]


class ImageListBuffer:
    _handle: MaaImageListBufferHandle
    _own: bool

    @property
    def c_handle(self) -> MaaImageListBufferHandle:
        return self._handle

    def __init__(self, c_handle: Optional[MaaImageListBufferHandle] = None):
        if not Library.initialized:
            raise RuntimeError(
                "Library not initialized, please call `library.open()` first."
            )
        self._set_api_properties()

        if c_handle:
            self._handle = c_handle
            self._own = False
        else:
            self._handle = Library.framework.MaaCreateImageListBuffer()
            self._own = True

        if not self._handle:
            raise RuntimeError("Failed to create image list buffer.")

    def __del__(self):
        if self._handle and self._own:
            Library.framework.MaaDestroyImageListBuffer(self._handle)

    def get(self) -> List[numpy.ndarray]:
        count = Library.framework.MaaGetImageListSize(self._handle)
        result = []
        for i in range(count):
            buff = Library.framework.MaaGetImageListAt(self._handle, i)
            img = ImageBuffer(buff).get()
            result.append(img)
        return result

    def set(self, value: List[numpy.ndarray]) -> bool:
        Library.framework.MaaClearImageList(self._handle)
        for img in value:
            buff = ImageBuffer()
            buff.set(img)
            if not Library.framework.MaaImageListAppend(self._handle, buff.c_handle):
                return False
        return True

    def append(self, value: numpy.ndarray) -> bool:
        buff = ImageBuffer()
        buff.set(value)
        return bool(Library.framework.MaaImageListAppend(self._handle, buff.c_handle))

    def remove(self, index: int) -> bool:
        return bool(Library.framework.MaaImageListRemove(self._handle, index))

    def clear(self) -> bool:
        return bool(Library.framework.MaaClearImageList(self._handle))

    _api_properties_initialized: bool = False

    @staticmethod
    def _set_api_properties():
        if ImageListBuffer._api_properties_initialized:
            return
        ImageListBuffer._api_properties_initialized = True

        Library.framework.MaaCreateImageListBuffer.restype = MaaImageListBufferHandle
        Library.framework.MaaCreateImageListBuffer.argtypes = None

        Library.framework.MaaDestroyImageListBuffer.restype = None
        Library.framework.MaaDestroyImageListBuffer.argtypes = [
            MaaImageListBufferHandle
        ]

        Library.framework.MaaIsImageListEmpty.restype = MaaBool
        Library.framework.MaaIsImageListEmpty.argtypes = [MaaImageListBufferHandle]

        Library.framework.MaaClearImageList.restype = MaaBool
        Library.framework.MaaClearImageList.argtypes = [MaaImageListBufferHandle]

        Library.framework.MaaGetImageListSize.restype = MaaSize
        Library.framework.MaaGetImageListSize.argtypes = [MaaImageListBufferHandle]

        Library.framework.MaaGetImageListAt.restype = MaaImageBufferHandle
        Library.framework.MaaGetImageListAt.argtypes = [
            MaaImageListBufferHandle,
            MaaSize,
        ]

        Library.framework.MaaImageListAppend.restype = MaaBool
        Library.framework.MaaImageListAppend.argtypes = [
            MaaImageListBufferHandle,
            MaaImageBufferHandle,
        ]

        Library.framework.MaaImageListRemove.restype = MaaBool
        Library.framework.MaaImageListRemove.argtypes = [
            MaaImageListBufferHandle,
            MaaSize,
        ]


class RectBuffer:
    _handle: MaaRectHandle
    _own: bool

    @property
    def c_handle(self) -> MaaRectHandle:
        return self._handle

    def __init__(self, c_handle: Optional[MaaRectHandle] = None):
        if not Library.initialized:
            raise RuntimeError(
                "Library not initialized, please call `library.open()` first."
            )

        self._set_api_properties()

        if c_handle:
            self._handle = c_handle
            self._own = False
        else:
            self._handle = Library.framework.MaaCreateRectBuffer()
            self._own = True

        if not self._handle:
            raise RuntimeError("Failed to create rect buffer.")

    def __del__(self):
        if self._handle and self._own:
            Library.framework.MaaDestroyRectBuffer(self._handle)

    def get(self) -> Rect:
        x = Library.framework.MaaGetRectX(self._handle)
        y = Library.framework.MaaGetRectY(self._handle)
        w = Library.framework.MaaGetRectW(self._handle)
        h = Library.framework.MaaGetRectH(self._handle)

        return Rect(x, y, w, h)

    def set(
        self,
        value: RectType,
    ) -> bool:
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
            Library.framework.MaaSetRect(
                self._handle, value[0], value[1], value[2], value[3]
            )
        )

    _api_properties_initialized: bool = False

    @staticmethod
    def _set_api_properties():
        if RectBuffer._api_properties_initialized:
            return
        RectBuffer._api_properties_initialized = True

        Library.framework.MaaCreateRectBuffer.restype = MaaRectHandle
        Library.framework.MaaCreateRectBuffer.argtypes = []

        Library.framework.MaaDestroyRectBuffer.restype = None
        Library.framework.MaaDestroyRectBuffer.argtypes = [MaaRectHandle]

        Library.framework.MaaGetRectX.restype = ctypes.c_int32
        Library.framework.MaaGetRectX.argtypes = [MaaRectHandle]

        Library.framework.MaaGetRectY.restype = ctypes.c_int32
        Library.framework.MaaGetRectY.argtypes = [MaaRectHandle]

        Library.framework.MaaGetRectW.restype = ctypes.c_int32
        Library.framework.MaaGetRectW.argtypes = [MaaRectHandle]

        Library.framework.MaaGetRectH.restype = ctypes.c_int32
        Library.framework.MaaGetRectH.argtypes = [MaaRectHandle]

        Library.framework.MaaSetRect.restype = MaaBool
        Library.framework.MaaSetRect.argtypes = [
            MaaRectHandle,
            ctypes.c_int32,
            ctypes.c_int32,
            ctypes.c_int32,
            ctypes.c_int32,
        ]
