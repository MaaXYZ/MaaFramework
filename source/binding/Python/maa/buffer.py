import ctypes
import numpy
from typing import Union, Optional

from .define import MaaBool
from .library import Library


class StringBuffer:
    _handle: ctypes.c_void_p

    def __init__(self):
        if not Library.initialized:
            raise RuntimeError(
                "Library not initialized, please call `library.open()` first."
            )

        self._set_api_properties()

        self._handle = Library.framework.MaaCreateStringBuffer()
        if not self._handle:
            raise RuntimeError("Failed to create string buffer.")

    def __del__(self):
        if self._handle:
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

    def __init__(self):
        if not Library.initialized:
            raise RuntimeError(
                "Library not initialized, please call `library.open()` first."
            )

        self._set_api_properties()

        self._handle = Library.framework.MaaCreateImageBuffer()
        if not self._handle:
            raise RuntimeError("Failed to create string buffer.")

    def __del__(self):
        if self._handle:
            Library.framework.MaaDestroyImageBuffer(self._handle)

    def get(self) -> numpy.ndarray:
        buff = Library.framework.MaaGetImageRawData(self._handle)
        width = int(Library.framework.MaaGetImageWidth(self._handle))
        height = int(Library.framework.MaaGetImageHeight(self._handle))

        return numpy.from_buffer(
            buff, dtype=numpy.uint8, count=height * width * 3
        ).reshape((height, width, 3))

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

        Library.framework.MaaGetImageRawData.restype = ctypes.c_void_p
        Library.framework.MaaGetImageRawData.argtypes = [ctypes.c_void_p]

        Library.framework.MaaGetImageWidth.restype = ctypes.c_int32
        Library.framework.MaaGetImageWidth.argtypes = [ctypes.c_void_p]

        Library.framework.MaaGetImageHeight.restype = ctypes.c_int32
        Library.framework.MaaGetImageHeight.argtypes = [ctypes.c_void_p]

        Library.framework.MaaGetImageType.restype = ctypes.c_int32
        Library.framework.MaaGetImageType.argtypes = [ctypes.c_void_p]

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

    def __init__(self):
        if not Library.initialized:
            raise RuntimeError(
                "Library not initialized, please call `library.open()` first."
            )

        self._set_api_properties()

        self._handle = Library.framework.MaaCreateRectBuffer()
        if not self._handle:
            raise RuntimeError("Failed to create string buffer.")

    def __del__(self):
        if self._handle:
            Library.framework.MaaDestroyRectBuffer(self._handle)

    def get(self) -> numpy.ndarray:
        x = Library.framework.MaaGetRectX(self._handle)
        y = Library.framework.MaaGetRectY(self._handle)
        w = Library.framework.MaaGetRectW(self._handle)
        h = Library.framework.MaaGetRectH(self._handle)

        return numpy.array([x, y, w, h], dtype=numpy.int32)

    def set(self, value: numpy.ndarray) -> bool:
        if not isinstance(value, numpy.ndarray):
            raise TypeError("value must be a numpy.ndarray")
        if value.ndim != 1:
            raise ValueError("value must be a 1D array")
        if value.shape[0] != 4:
            raise ValueError("value must have 4 elements")
        if value.dtype != numpy.int32:
            raise ValueError("value must be of type numpy.int32")

        return (
            bool(Library.framework.MaaSetRectX(self._handle, value[0]))
            and bool(Library.framework.MaaSetRectY(self._handle, value[1]))
            and bool(Library.framework.MaaSetRectW(self._handle, value[2]))
            and bool(Library.framework.MaaSetRectH(self._handle, value[3]))
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

        Library.framework.MaaSetRectX.restype = MaaBool
        Library.framework.MaaSetRectX.argtypes = [ctypes.c_void_p, ctypes.c_int32]

        Library.framework.MaaSetRectY.restype = MaaBool
        Library.framework.MaaSetRectY.argtypes = [ctypes.c_void_p, ctypes.c_int32]

        Library.framework.MaaSetRectW.restype = MaaBool
        Library.framework.MaaSetRectW.argtypes = [ctypes.c_void_p, ctypes.c_int32]

        Library.framework.MaaSetRectH.restype = MaaBool
        Library.framework.MaaSetRectH.restype = MaaBool
