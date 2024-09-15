import ctypes
import ctypes.util
import pathlib
import platform
from typing import Optional, Union

from .define import *


class Library:

    initialized = False

    @staticmethod
    def open(path: Union[pathlib.Path, str]) -> Optional[str]:
        platform_values = {
            "windows": ("MaaFramework.dll", "MaaToolkit.dll"),
            "darwin": ("libMaaFramework.dylib", "libMaaToolkit.dylib"),
            "linux": ("libMaaFramework.so", "libMaaToolkit.so"),
        }

        platform_type = platform.system().lower()

        if platform_type == "windows":
            lib_import = ctypes.WinDLL
        else:
            lib_import = ctypes.CDLL

        try:
            Library.framework_libpath = (
                pathlib.Path(path) / platform_values[platform_type][0]
            )
            Library.framework = lib_import(str(Library.framework_libpath))
        except OSError:
            Library.framework_libpath = ctypes.util.find_library("MaaFramework")
            Library.framework = lib_import(str(Library.framework_libpath))

        try:
            Library.toolkit_libpath = (
                pathlib.Path(path) / platform_values[platform_type][1]
            )
            Library.toolkit = lib_import(str(Library.toolkit_libpath))
        except OSError:
            Library.toolkit_libpath = ctypes.util.find_library("MaaToolkit")
            Library.toolkit = lib_import(str(Library.toolkit_libpath))

        if not Library.framework or not Library.toolkit:
            Library.initialized = False
            return None

        Library._set_api_properties()
        Library.initialized = True

        return Library.version()

    @staticmethod
    def version() -> str:
        if not Library.initialized:
            raise RuntimeError(
                "Library not initialized, please call `library.open()` first."
            )

        return Library.framework.MaaVersion().decode()

    @staticmethod
    def _set_api_properties():
        Library.framework.MaaVersion.restype = ctypes.c_char_p
        Library.framework.MaaVersion.argtypes = []
