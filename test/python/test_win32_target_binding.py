"""Exercise cold startup and window replacement against the real Win32 backend."""

import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import time
import unittest


VK_F23 = 0x86
VK_F24 = 0x87


def run_window_fixture(directory):
    import ctypes
    from ctypes import wintypes as w

    # Plain top-level windows let the test observe exactly which HWND received WM_KEYUP.
    # The fixture uses PostMessage input throughout; it never injects a desktop keystroke.
    class WindowClass(ctypes.Structure):
        _fields_ = [
            ("style", w.UINT), ("wnd_proc", ctypes.c_void_p), ("class_extra", ctypes.c_int),
            ("window_extra", ctypes.c_int), ("instance", w.HINSTANCE), ("icon", w.HICON),
            ("cursor", w.HANDLE), ("background", w.HBRUSH), ("menu", w.LPCWSTR), ("name", w.LPCWSTR),
        ]

    class Message(ctypes.Structure):
        _fields_ = w.MSG._fields_ + [("lPrivate", w.DWORD)]

    user32 = ctypes.windll.user32
    kernel32 = ctypes.windll.kernel32
    kernel32.GetModuleHandleW.argtypes = [w.LPCWSTR]
    kernel32.GetModuleHandleW.restype = w.HMODULE
    instance = kernel32.GetModuleHandleW(None)
    cls = WindowClass()
    cls.wnd_proc = ctypes.cast(user32.DefWindowProcW, ctypes.c_void_p)
    cls.instance = instance
    cls.name = "MaaBindTargetFixture"
    user32.RegisterClassW.argtypes = [ctypes.POINTER(WindowClass)]
    if not user32.RegisterClassW(ctypes.byref(cls)):
        raise ctypes.WinError()

    user32.CreateWindowExW.argtypes = [
        w.DWORD, w.LPCWSTR, w.LPCWSTR, w.DWORD, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int,
        w.HWND, w.HMENU, w.HINSTANCE, w.LPVOID,
    ]
    user32.CreateWindowExW.restype = w.HWND
    user32.DestroyWindow.argtypes = [w.HWND]
    user32.PeekMessageW.argtypes = [ctypes.POINTER(Message), w.HWND, w.UINT, w.UINT, w.UINT]
    user32.DispatchMessageW.argtypes = [ctypes.POINTER(Message)]
    user32.DispatchMessageW.restype = ctypes.c_ssize_t

    def open_window(role, width):
        # WS_EX_NOACTIVATE, WS_POPUP | WS_VISIBLE
        hwnd = user32.CreateWindowExW(
            0x08000000, cls.name, f"{directory.name}-{role}", 0x90000000, 20, 20, width, 120,
            None, None, instance, None,
        )
        if not hwnd:
            raise ctypes.WinError()
        return hwnd

    game = open_window("Game", 160)
    launcher = open_window("Launcher", 240)
    replacement = None
    msg = Message()
    deadline = time.monotonic() + 60
    while directory.is_dir() and time.monotonic() < deadline:
        while user32.PeekMessageW(ctypes.byref(msg), None, 0, 0, 1):
            if replacement and msg.hWnd == replacement and msg.message == 0x0101:  # WM_KEYUP
                if msg.wParam == VK_F24:
                    (directory / "stray-up").touch()
                if msg.wParam == VK_F23:
                    (directory / "received-up").touch()
            user32.DispatchMessageW(ctypes.byref(msg))
        if launcher and (directory / "close-launcher").exists():
            (directory / "close-launcher").unlink()
            user32.DestroyWindow(launcher)
            launcher = None
            (directory / "closed").touch()
        if not launcher and (directory / "reopen-launcher").exists():
            replacement = launcher = open_window("Launcher", 320)
        time.sleep(0.01)
    if launcher:
        user32.DestroyWindow(launcher)
    user32.DestroyWindow(game)


@unittest.skipUnless(sys.platform == "win32", "requires the Win32 controller")
class Win32TargetBindingTest(unittest.TestCase):
    def test_bind_desktop_without_capture(self):
        from maa.controller import Win32Controller
        from maa.define import MaaWin32InputMethodEnum as Input, MaaWin32ScreencapMethodEnum as Capture

        controller = Win32Controller(None, Capture.Null, Input.Null, Input.Null, deferred_binding=True)
        self.assertTrue(controller.post_connection().wait().succeeded)
        self.assertTrue(controller.post_bind_target({"win32": {"method": "Desktop"}}).wait().succeeded)
        self.assertEqual(controller.info["target_state"], "Desktop")

    def test_start_bind_and_replace_window(self):
        from maa.controller import Win32Controller
        from maa.define import MaaWin32InputMethodEnum as Input, MaaWin32ScreencapMethodEnum as Capture
        from maa.resource import Resource
        from maa.tasker import Tasker

        with tempfile.TemporaryDirectory(prefix="MaaBindTarget-") as tmp:
            root = Path(tmp)
            title = re.escape(root.name)

            def target(suffix, timeout=0):
                return {"win32": {"method": "Window", "window_regex": f"^{title}-{suffix}$"}, "bind_timeout": timeout}

            controller = Win32Controller(None, Capture.GDI, Input.PostMessage, Input.PostMessage, deferred_binding=True)
            self.assertTrue(controller.post_connection().wait().succeeded)
            self.assertTrue(controller.connected)
            session_uuid = controller.uuid
            self.assertEqual(controller.info["target_state"], "Unbound")
            self.assertTrue(controller.post_screencap().wait().failed)
            self.assertTrue(controller.post_bind_target(target("Launcher")).wait().failed)

            command = subprocess.list2cmdline([sys.executable, str(Path(__file__).resolve()), "--fixture", str(root)])
            pipeline = {
                "Launch": {"action": "StartApp", "package": command, "next": ["Bind"]},
                "Bind": {"action": {"type": "BindTarget", "param": target("Launcher", 15000)}, "next": ["Read"]},
                "Read": {"recognition": "ColorMatch", "lower": [0, 0, 0], "upper": [255, 255, 255], "roi": [0, 0, 16, 16]},
            }
            (root / "pipeline").mkdir()
            (root / "pipeline" / "startup.json").write_text(json.dumps(pipeline), encoding="utf-8")
            resource = Resource()
            self.assertTrue(resource.post_bundle(root).wait().succeeded)
            tasker = Tasker()
            self.assertTrue(tasker.bind(resource, controller))
            self.assertTrue(tasker.post_task("Launch").wait().succeeded)
            self.assertEqual(controller.info["target_state"], "Window")
            self.assertGreater(controller.cached_image.size, 0)
            old_hwnd = controller.info["hwnd"]
            old_size = controller.resolution
            old_shape = controller.cached_image.shape
            self.assertTrue(controller.post_bind_target(target("Launcher")).wait().succeeded)
            self.assertEqual(controller.resolution, old_size)
            self.assertEqual(controller.cached_image.shape, old_shape)

            legacy = Win32Controller(old_hwnd, Capture.GDI, Input.PostMessage, Input.PostMessage)
            self.assertTrue(legacy.post_connection().wait().succeeded)
            self.assertTrue(legacy.post_screencap().wait().succeeded)

            wrong_class = target("Launcher")
            wrong_class["win32"]["class_regex"] = "^MaaNonexistentWindowClass$"
            for invalid in (target("Missing"), wrong_class, target("(Launcher|Game)"), {"win32": {"method": "Desktop"}}):
                with self.subTest(target=invalid):
                    self.assertTrue(controller.post_bind_target(invalid).wait().failed)
                    self.assertEqual(controller.info["hwnd"], old_hwnd)
                    self.assertEqual(controller.resolution, old_size)
                    self.assertEqual(controller.cached_image.shape, old_shape)
                    self.assertTrue(controller.post_screencap().wait().succeeded)

            self.assertTrue(controller.post_key_down(VK_F24).wait().succeeded)
            (root / "close-launcher").touch()
            deadline = time.monotonic() + 5
            while not (root / "closed").exists():
                self.assertLess(time.monotonic(), deadline, "fixture did not close the launcher")
                time.sleep(0.03)
            self.assertTrue(controller.connected)
            self.assertEqual(controller.info["target_state"], "Lost")
            self.assertTrue(controller.post_key_up(VK_F23).wait().failed)
            self.assertTrue(controller.post_screencap().wait().failed)

            (root / "reopen-launcher").touch()
            self.assertTrue(controller.post_bind_target(target("Launcher", 5000)).wait().succeeded)
            self.assertEqual(controller.resolution, (0, 0))
            self.assertTrue(controller.post_screencap().wait().succeeded)
            self.assertGreater(controller.resolution[0], old_size[0])
            self.assertEqual(controller.uuid, session_uuid)

            # The fixture reports a different key-up as a barrier after all queued rebind cleanup.
            self.assertTrue(controller.post_key_down(VK_F23).wait().succeeded)
            self.assertTrue(controller.post_key_up(VK_F23).wait().succeeded)
            deadline = time.monotonic() + 5
            while not (root / "received-up").exists():
                self.assertLess(time.monotonic(), deadline, "replacement did not receive the test key-up")
                time.sleep(0.03)
            self.assertFalse((root / "stray-up").exists(), "replacement received a release belonging to the old window")

            pending = controller.post_bind_target(target("Missing", 20000))
            deadline = time.monotonic() + 5
            while not pending.running:
                self.assertFalse(pending.done)
                self.assertLess(time.monotonic(), deadline, "BindTarget did not start")
                time.sleep(0.03)
            started = time.monotonic()
            self.assertTrue(tasker.post_stop().wait().succeeded)
            self.assertTrue(pending.wait().failed)
            self.assertLess(time.monotonic() - started, 10)
            self.assertEqual(controller.info["target_state"], "Window")


if __name__ == "__main__":
    if len(sys.argv) == 3 and sys.argv[1] == "--fixture":
        run_window_fixture(Path(sys.argv[2]))
    else:
        unittest.main()
