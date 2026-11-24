"""In-memory controller used by the local and cross-process contract tests."""

import numpy

from maa.controller import CustomController


class RecordingController(CustomController):
    """Return fixed pixels and record calls made through the native controller."""

    def __init__(self, *, raw_size=True):
        self.calls = []
        self.fail = set()
        self.is_connected = False
        self.image = numpy.arange(24 * 32 * 3, dtype=numpy.uint8).reshape(24, 32, 3)
        super().__init__()
        if raw_size:
            self.set_screenshot_use_raw_size(True)

    def record(self, name, *args):
        self.calls.append((name, *args))
        return name not in self.fail

    def connect(self):
        self.is_connected = self.record("connect")
        return self.is_connected

    def connected(self):
        return self.is_connected

    def request_uuid(self):
        return "api-contract-controller"

    def get_features(self):
        return 0

    def start_app(self, intent):
        return self.record("start_app", intent)

    def stop_app(self, intent):
        return self.record("stop_app", intent)

    def screencap(self):
        self.record("screencap")
        return self.image

    def click(self, x, y):
        return self.record("click", x, y)

    def swipe(self, x1, y1, x2, y2, duration):
        return self.record("swipe", x1, y1, x2, y2, duration)

    def touch_down(self, contact, x, y, pressure):
        return self.record("touch_down", contact, x, y, pressure)

    def touch_move(self, contact, x, y, pressure):
        return self.record("touch_move", contact, x, y, pressure)

    def touch_up(self, contact):
        return self.record("touch_up", contact)

    def click_key(self, keycode):
        return self.record("click_key", keycode)

    def key_down(self, keycode):
        return self.record("key_down", keycode)

    def key_up(self, keycode):
        return self.record("key_up", keycode)

    def input_text(self, text):
        return self.record("input_text", text)

    def scroll(self, dx, dy):
        return self.record("scroll", dx, dy)

    def relative_move(self, dx, dy):
        return self.record("relative_move", dx, dy)

    def shell(self, cmd, timeout):
        if not self.record("shell", cmd, timeout):
            return None
        return f"stdout:{cmd}\n结果🐱"

    def inactive(self):
        return self.record("inactive")

    def get_custom_info(self):
        return {"fixture": "接口测试", "nested": {"enabled": True}}
