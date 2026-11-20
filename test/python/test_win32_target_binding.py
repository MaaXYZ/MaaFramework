"""Exercise cold startup and window replacement against the real Win32 backend."""

import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import time
import unittest


def run_window_fixture(directory):
    import tkinter as tk

    # Keep the game window alive while the test destroys and recreates the launcher.
    game = tk.Tk()
    game.title(f"{directory.name}-Game")
    game.geometry("160x120")
    game.update()

    def open_launcher(width):
        window = tk.Toplevel(game)
        window.title(f"{directory.name}-Launcher")
        window.geometry(f"{width}x120")
        return window

    launcher = open_launcher(240)

    def check_requests():
        nonlocal launcher
        if not directory.is_dir():
            game.destroy()
            return
        if launcher and (directory / "close-launcher").exists():
            (directory / "close-launcher").unlink()
            launcher.destroy()
            launcher = None
            (directory / "closed").touch()
        if not launcher and (directory / "reopen-launcher").exists():
            launcher = open_launcher(320)
        game.after(30, check_requests)

    game.after(30, check_requests)
    game.after(60000, game.destroy)
    game.mainloop()


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

            (root / "close-launcher").touch()
            deadline = time.monotonic() + 5
            while not (root / "closed").exists():
                self.assertLess(time.monotonic(), deadline, "fixture did not close the launcher")
                time.sleep(0.03)
            self.assertTrue(controller.connected)
            self.assertEqual(controller.info["target_state"], "Lost")
            self.assertTrue(controller.post_screencap().wait().failed)

            (root / "reopen-launcher").touch()
            self.assertTrue(controller.post_bind_target(target("Launcher", 5000)).wait().succeeded)
            self.assertEqual(controller.resolution, (0, 0))
            self.assertTrue(controller.post_screencap().wait().succeeded)
            self.assertGreater(controller.resolution[0], old_size[0])
            self.assertEqual(controller.uuid, session_uuid)

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
