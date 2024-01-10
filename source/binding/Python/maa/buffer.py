import ctypes
import numpy
from typing import List, Tuple, Union, Optional
from PIL import Image

from .define import *
from .library import Library


class StringBuffer:
    _handle: MaaStringBufferHandle
    _own: bool

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


class ImageBuffer:
    _handle: MaaImageBufferHandle
    _own: bool

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
        w = Library.framework.MaaGetImageWidth(self._handle)
        h = Library.framework.MaaGetImageHeight(self._handle)
        # t = Library.framework.MaaGetImageType(self._handle)
        return numpy.ctypeslib.as_array(
            ctypes.cast(buff, ctypes.POINTER(ctypes.c_uint8)), shape=(h, w, 3)
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


class RectBuffer:
    _handle: MaaRectHandle
    _own: bool

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

    def get(self) -> Tuple[int, int, int, int]:
        x = Library.framework.MaaGetRectX(self._handle)
        y = Library.framework.MaaGetRectY(self._handle)
        w = Library.framework.MaaGetRectW(self._handle)
        h = Library.framework.MaaGetRectH(self._handle)

        return x, y, w, h

    def set(
        self,
        value: Union[
            numpy.ndarray,
            Tuple[int, int, int, int],
            List[int],
        ],
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
        else:
            raise TypeError("value must be a numpy.ndarray, tuple or list")

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
