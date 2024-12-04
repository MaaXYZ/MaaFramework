import ctypes
import ctypes.util
import pathlib
import platform

from .define import *


class Library:

    @staticmethod
    def open(path: pathlib.Path):
        if not path.exists():
            raise FileNotFoundError(f"`{path}` does not exist.")

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

        Library.framework_libpath = path / platform_values[platform_type][0]
        Library.framework = lib_import(str(Library.framework_libpath))

        Library.toolkit_libpath = path / platform_values[platform_type][1]
        Library.toolkit = lib_import(str(Library.toolkit_libpath))

        Library._set_api_properties()

    @staticmethod
    def version() -> str:
        return Library.framework.MaaVersion().decode()

    @staticmethod
    def _set_api_properties():
        Library.framework.MaaVersion.restype = ctypes.c_char_p
        Library.framework.MaaVersion.argtypes = []
