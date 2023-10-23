import ctypes
import ctypes.util
import pathlib
import os
import platform
from typing import Union


class Library:
    @classmethod
    def open(cls, path: Union[pathlib.Path, str], toolkit: bool = True) -> bool:
        """
        Open the library at the given path.

        :param path: The path to the library.
        :return: True if the library was successfully opened, False otherwise.
        """

        platform_values = {
            "windows": {
                "framework_libpath": "MaaFramework.dll",
                "toolkit_libpath": "MaaToolKit.dll",
                "environ_var": "PATH",
            },
            "darwin": {
                "framework_libpath": "libMaaFramework.dylib",
                "toolkit_libpath": "libMaaToolKit.dylib",
                "environ_var": "DYLD_LIBRARY_PATH",
            },
            "linux": {
                "framework_libpath": "libMaaFramework.so",
                "toolkit_libpath": "libMaaToolKit.so",
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

        cls.initialized = True

        if toolkit:
            try:
                cls.toolkit_libpath = (
                    pathlib.Path(path)
                    / platform_values[platform_type]["toolkit_libpath"]
                )
                cls.toolkit = lib_import(str(cls.toolkit_libpath))
            except OSError:
                cls.toolkit_libpath = ctypes.util.find_library("MaaToolKit")
                cls.toolkit = lib_import(str(cls.toolkit_libpath))
        else:
            cls.toolkit = None
            cls.toolkit_libpath = None

        return True
