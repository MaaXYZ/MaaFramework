import ctypes
import numpy
import io
from typing import Union, Optional
from PIL import Image
import io

from .define import MaaBool
from .library import Library


class StringBuffer:
    _handle: ctypes.c_void_p
    _own: bool

    def __init__(self):
        if not Library.initialized:
            raise RuntimeError(
                "Library not initialized, please call `library.open()` first."
            )

        self._set_api_properties()

        self._handle = Library.framework.MaaCreateStringBuffer()
        self._own = True
        if not self._handle:
            raise RuntimeError("Failed to create string buffer.")

    def __init__(self, c_string_handle: ctypes.c_void_p):
        if not Library.initialized:
            raise RuntimeError(
                "Library not initialized, please call `library.open()` first."
            )
        if not c_string_handle:
            raise ValueError("c_string_handle must not be None")

        self._set_api_properties()

        self._handle = c_string_handle
        self._own = False

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

        Library.framework.MaaCreateStringBuffer.restype = ctypes.c_void_p
        Library.framework.MaaCreateStringBuffer.argtypes = []

        Library.framework.MaaDestroyStringBuffer.restype = None
        Library.framework.MaaDestroyStringBuffer.argtypes = [ctypes.c_void_p]

        Library.framework.MaaIsStringEmpty.restype = MaaBool
        Library.framework.MaaIsStringEmpty.argtypes = [ctypes.c_void_p]

        Library.framework.MaaClearString.restype = MaaBool
        Library.framework.MaaClearString.argtypes = [ctypes.c_void_p]

        Library.framework.MaaGetString.restype = ctypes.c_char_p
        Library.framework.MaaGetString.argtypes = [ctypes.c_void_p]

        Library.framework.MaaGetStringSize.restype = ctypes.c_size_t
        Library.framework.MaaGetStringSize.argtypes = [ctypes.c_void_p]

        Library.framework.MaaSetString.restype = MaaBool
        Library.framework.MaaSetString.argtypes = [ctypes.c_void_p, ctypes.c_char_p]

        Library.framework.MaaSetStringEx.restype = MaaBool
        Library.framework.MaaSetStringEx.argtypes = [
            ctypes.c_void_p,
            ctypes.c_char_p,
            ctypes.c_size_t,
        ]


class ImageBuffer:
    _handle: ctypes.c_void_p
    _own: bool

    def __init__(self):
        if not Library.initialized:
            raise RuntimeError(
                "Library not initialized, please call `library.open()` first."
            )

        self._set_api_properties()

        self._handle = Library.framework.MaaCreateImageBuffer()
        self._own = True
        if not self._handle:
            raise RuntimeError("Failed to create string buffer.")

    def __init__(self, c_image_handle: ctypes.c_void_p):
        if not Library.initialized:
            raise RuntimeError(
                "Library not initialized, please call `library.open()` first."
            )
        if not c_image_handle:
            raise ValueError("c_image_handle must not be None")

        self._set_api_properties()

        self._handle = c_image_handle
        self._own = False

    def __del__(self):
        if self._handle and self._own:
            Library.framework.MaaDestroyImageBuffer(self._handle)

    def get(self) -> numpy.ndarray:
        buffer = Library.framework.MaaGetImageEncoded(self._handle)
        size = int(Library.framework.MaaGetImageEncodedSize(self._handle))
        png_data = ctypes.string_at(buffer, size)
        img = Image.open(io.BytesIO(png_data))
        return numpy.array(img)

    def set(self, value: numpy.ndarray) -> bool:
        if not isinstance(value, numpy.ndarray):
            raise TypeError("value must be a numpy.ndarray")
        if value.ndim != 3:
            raise ValueError("value must be a 3D array")
        if value.shape[2] != 3:
            raise ValueError("value must have 3 channels")
        if value.dtype != numpy.uint8:
            raise ValueError("value must be of type numpy.uint8")

        return bool(
            Library.framework.MaaSetImageRawData(
                self._handle,
                value.ctypes.data_as(ctypes.c_void_p),
                value.shape[1],
                value.shape[0],
                value.shape[2],
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

        Library.framework.MaaCreateImageBuffer.restype = ctypes.c_void_p
        Library.framework.MaaCreateImageBuffer.argtypes = []

        Library.framework.MaaDestroyImageBuffer.restype = None
        Library.framework.MaaDestroyImageBuffer.argtypes = [ctypes.c_void_p]

        Library.framework.MaaGetImageEncoded.restype = ctypes.POINTER(ctypes.c_uint8)
        Library.framework.MaaGetImageEncoded.argtypes = [ctypes.c_void_p]

        Library.framework.MaaGetImageEncodedSize.restype = ctypes.c_size_t
        Library.framework.MaaGetImageEncodedSize.argtypes = [ctypes.c_void_p]

        Library.framework.MaaSetImageRawData.restype = MaaBool
        Library.framework.MaaSetImageRawData.argtypes = [
            ctypes.c_void_p,
            ctypes.c_void_p,
            ctypes.c_int32,
            ctypes.c_int32,
            ctypes.c_int32,
        ]

        Library.framework.MaaIsImageEmpty.restype = MaaBool
        Library.framework.MaaIsImageEmpty.argtypes = [ctypes.c_void_p]

        Library.framework.MaaClearImage.restype = MaaBool
        Library.framework.MaaClearImage.argtypes = [ctypes.c_void_p]


class RectBuffer:
    _handle: ctypes.c_void_p
    _own: bool

    def __init__(self):
        if not Library.initialized:
            raise RuntimeError(
                "Library not initialized, please call `library.open()` first."
            )

        self._set_api_properties()

        self._handle = Library.framework.MaaCreateRectBuffer()
        if not self._handle:
            raise RuntimeError("Failed to create string buffer.")

    def __init__(self, c_rect_handle: ctypes.c_void_p):
        if not Library.initialized:
            raise RuntimeError(
                "Library not initialized, please call `library.open()` first."
            )
        if not c_rect_handle:
            raise ValueError("c_rect_handle must not be None")

        self._set_api_properties()

        self._handle = c_rect_handle
        self._own = False

    def __del__(self):
        if self._handle and self._own:
            Library.framework.MaaDestroyRectBuffer(self._handle)

    def get(self) -> numpy.ndarray:
        x = Library.framework.MaaGetRectX(self._handle)
        y = Library.framework.MaaGetRectY(self._handle)
        w = Library.framework.MaaGetRectW(self._handle)
        h = Library.framework.MaaGetRectH(self._handle)

        return numpy.array([x, y, w, h], dtype=numpy.int32)

    def set(self, value: Union[numpy.ndarray, tuple, list]) -> bool:
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

        Library.framework.MaaCreateRectBuffer.restype = ctypes.c_void_p
        Library.framework.MaaCreateRectBuffer.argtypes = []

        Library.framework.MaaDestroyRectBuffer.restype = None
        Library.framework.MaaDestroyRectBuffer.argtypes = [ctypes.c_void_p]

        Library.framework.MaaGetRectX.restype = ctypes.c_int32
        Library.framework.MaaGetRectX.argtypes = [ctypes.c_void_p]

        Library.framework.MaaGetRectY.restype = ctypes.c_int32
        Library.framework.MaaGetRectY.argtypes = [ctypes.c_void_p]

        Library.framework.MaaGetRectW.restype = ctypes.c_int32
        Library.framework.MaaGetRectW.argtypes = [ctypes.c_void_p]

        Library.framework.MaaGetRectH.restype = ctypes.c_int32
        Library.framework.MaaGetRectH.argtypes = [ctypes.c_void_p]

        Library.framework.MaaSetRect.restype = MaaBool
        Library.framework.MaaSetRect.argtypes = [
            ctypes.c_void_p,
            ctypes.c_int32,
            ctypes.c_int32,
            ctypes.c_int32,
            ctypes.c_int32,
        ]
