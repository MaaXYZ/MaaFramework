import ctypes
import ctypes.util
import pathlib
import os
import platform
from typing import Union, Optional

from .define import *


class Library:
    @classmethod
    def open(cls, path: Union[pathlib.Path, str]) -> Optional[str]:
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

        try:
            cls.toolkit_libpath = pathlib.Path(path) / platform_values[platform_type][1]
            cls.toolkit = lib_import(str(cls.toolkit_libpath))
        except OSError:
            cls.toolkit_libpath = ctypes.util.find_library("MaaToolkit")
            cls.toolkit = lib_import(str(cls.toolkit_libpath))

        if not cls.framework or not cls.toolkit:
            cls.initialized = False
            return None

        cls._set_api_properties()
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

        return cls.framework.MaaVersion().decode("utf-8")

    @classmethod
    def set_log_dir(cls, path: Union[pathlib.Path, str]) -> "Library":
        """
        Set the log directory.

        :param path: The path to the log directory.
        """

        if not cls.initialized:
            raise RuntimeError(
                "Library not initialized, please call `library.open()` first."
            )

        cls.framework.MaaSetGlobalOption(
            MaaGlobalOptionEnum.LogDir,
            str(path).encode("utf-8"),
            len(path),
        )
        return cls

    @classmethod
    def set_save_draw(cls, save_draw: bool) -> "Library":
        """
        Set whether to save draw.

        :param save_draw: Whether to save draw.
        """

        if not cls.initialized:
            raise RuntimeError(
                "Library not initialized, please call `library.open()` first."
            )

        cbool = ctypes.c_bool(save_draw)
        cls.framework.MaaSetGlobalOption(
            MaaGlobalOptionEnum.SaveDraw,
            ctypes.pointer(cbool),
            ctypes.sizeof(ctypes.c_bool),
        )
        return cls

    @classmethod
    def set_recording(cls, recording: bool) -> "Library":
        """
        Set whether to dump all screenshots and actions.

        :param recording: Whether to dump all screenshots and actions.
        """

        if not cls.initialized:
            raise RuntimeError(
                "Library not initialized, please call `library.open()` first."
            )

        cbool = ctypes.c_bool(recording)
        cls.framework.MaaSetGlobalOption(
            MaaGlobalOptionEnum.Recording,
            ctypes.pointer(cbool),
            ctypes.sizeof(ctypes.c_bool),
        )
        return cls

    @classmethod
    def set_stdout_level(cls, level: MaaLoggingLevelEunm) -> "Library":
        """
        Set the level of log output to stdout.

        :param level: The level of log output to stdout.
        """

        if not cls.initialized:
            raise RuntimeError(
                "Library not initialized, please call `library.open()` first."
            )

        cbool = ctypes.c_bool(level)
        cls.framework.MaaSetGlobalOption(
            MaaGlobalOptionEnum.StdoutLevel,
            ctypes.pointer(cbool),
            ctypes.sizeof(MaaLoggingLevel),
        )
        return cls

    @classmethod
    def set_show_hit_draw(cls, show_hit_draw: bool) -> "Library":
        """
        Set whether to show hit draw.

        :param show_hit_draw: Whether to show hit draw.
        """

        if not cls.initialized:
            raise RuntimeError(
                "Library not initialized, please call `library.open()` first."
            )

        cbool = ctypes.c_bool(show_hit_draw)
        cls.framework.MaaSetGlobalOption(
            MaaGlobalOptionEnum.ShowHitDraw,
            ctypes.pointer(cbool),
            ctypes.sizeof(ctypes.c_bool),
        )
        return cls

    @classmethod
    def set_debug_message(cls, debug_message: bool) -> "Library":
        """
        Set whether to callback debug message.

        :param debug_message: Whether to callback debug message.
        """

        if not cls.initialized:
            raise RuntimeError(
                "Library not initialized, please call `library.open()` first."
            )

        cbool = ctypes.c_bool(debug_message)
        cls.framework.MaaSetGlobalOption(
            MaaGlobalOptionEnum.DebugMessage,
            ctypes.pointer(cbool),
            ctypes.sizeof(ctypes.c_bool),
        )
        return cls

    @classmethod
    def _set_api_properties(cls):
        cls.framework.MaaVersion.restype = MaaStringView
        cls.framework.MaaVersion.argtypes = None

        cls.framework.MaaSetGlobalOption.restype = MaaBool
        cls.framework.MaaSetGlobalOption.argtypes = [
            MaaGlobalOption,
            MaaOptionValue,
            MaaOptionValueSize,
        ]
