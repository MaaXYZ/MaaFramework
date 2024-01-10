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
            "windows": {
                "framework_libpath": "MaaFramework.dll",
                "toolkit_libpath": "MaaToolkit.dll",
                "environ_var": "PATH",
            },
            "darwin": {
                "framework_libpath": "libMaaFramework.dylib",
                "toolkit_libpath": "libMaaToolkit.dylib",
                "environ_var": "DYLD_LIBRARY_PATH",
            },
            "linux": {
                "framework_libpath": "libMaaFramework.so",
                "toolkit_libpath": "libMaaToolkit.so",
                "environ_var": "LD_LIBRARY_PATH",
            },
        }

        platform_type = platform.system().lower()
        environ_var = platform_values[platform_type]["environ_var"]
        environ = os.environ[environ_var]
        try:
            environ += os.pathsep + str(path)
        except KeyError:
            environ = str(path)

        if platform_type == "windows":
            lib_import = ctypes.WinDLL
        else:
            lib_import = ctypes.CDLL

        try:
            cls.framework_libpath = (
                pathlib.Path(path) / platform_values[platform_type]["framework_libpath"]
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
                    pathlib.Path(path)
                    / platform_values[platform_type]["toolkit_libpath"]
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
