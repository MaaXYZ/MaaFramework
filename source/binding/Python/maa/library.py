import ctypes
import ctypes.util
import pathlib
import os
import platform
from typing import Union, Optional

from .define import MaaStringView


class Library:
    @classmethod
    def open(
        cls, path: Union[pathlib.Path, str], toolkit: bool = True
    ) -> Optional[str]:
        """
        Open the library at the given path.

        :param path: The path to the library.
        :return: True if the library was successfully opened, False otherwise.
        """

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
            cls.framework_libpath = (
                pathlib.Path(path) / platform_values[platform_type][0]
            )
            cls.framework = lib_import(str(cls.framework_libpath))
        except OSError:
            cls.framework_libpath = ctypes.util.find_library("MaaFramework")
            cls.framework = lib_import(str(cls.framework_libpath))

        if not cls.framework:
            cls.initialized = False
            return None

        if toolkit:
            try:
                cls.toolkit_libpath = (
                    pathlib.Path(path) / platform_values[platform_type][1]
                )
                cls.toolkit = lib_import(str(cls.toolkit_libpath))
            except OSError:
                cls.toolkit_libpath = ctypes.util.find_library("MaaToolkit")
                cls.toolkit = lib_import(str(cls.toolkit_libpath))
        else:
            cls.toolkit = None
            cls.toolkit_libpath = None

        cls.initialized = True

        return cls.version()

    @classmethod
    def version(cls) -> str:
        """
        Get the version of the library.

        :return: The version of the library.
        """

        if not cls.initialized:
            raise RuntimeError(
                "Library not initialized, please call `library.open()` first."
            )

        cls.framework.MaaVersion.restype = MaaStringView
        cls.framework.MaaVersion.argtypes = None

        return cls.framework.MaaVersion().decode("utf-8")
