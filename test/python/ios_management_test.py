"""Offline tests for the native-worker / Python callback boundary. No device is accessed."""

import gc
import json
import threading
import unittest
from unittest.mock import patch

from maa.define import MaaBool, MaaStringBufferHandle
from maa.library import Library
from maa.toolkit import Toolkit, _IOSCancelCallback, _IOSEventCallback


class FakeBuffer:
    instances = {}

    def __init__(self):
        self._handle = id(self)
        self.text = ""
        self.instances[self._handle] = self

    def get(self):
        return self.text


class NativeFunction:
    def __init__(self, name, api):
        self.name = name
        self.api = api

    def __call__(self, options, on_event, cancelled, userdata, result, credential=None):
        self.api.calls.append((self.name, json.loads(options.decode("utf-8"))))
        state = {}

        def worker():
            gc.collect()
            state["worker"] = threading.get_ident()
            state["cancelled"] = bool(cancelled(userdata))
            state["has_event_callback"] = bool(on_event)
            if not state["cancelled"] and on_event:
                state["accepted"] = bool(on_event('{"type":"pin","pin":"012345"}'.encode(), userdata))
                state["cancelled"] = bool(cancelled(userdata))

        thread = threading.Thread(target=worker)
        thread.start()
        thread.join()
        self.api.state = state
        FakeBuffer.instances[result].text = json.dumps({
            "success": not state["cancelled"], "cancelled": state["cancelled"],
            "error": "" if not state["cancelled"] else "cancelled", "label": "中文",
        })
        if credential is not None:
            FakeBuffer.instances[credential].text = "opaque-test-credential" if not state["cancelled"] else ""
        return not state["cancelled"]


class NativeAPI:
    def __init__(self):
        self.functions = {}
        self.calls = []
        self.state = {}

    def __getattr__(self, name):
        return self.functions.setdefault(name, NativeFunction(name, self))


class IOSManagementBindingTest(unittest.TestCase):
    def setUp(self):
        self.api = NativeAPI()
        self.toolkit_patch = patch.object(Library, "toolkit", return_value=self.api)
        self.init_patch = patch.object(Toolkit, "_set_api_properties")
        self.buffer_patch = patch("maa.buffer.StringBuffer", FakeBuffer)
        for item in (self.toolkit_patch, self.init_patch, self.buffer_patch):
            item.start()
            self.addCleanup(item.stop)
        self.addCleanup(FakeBuffer.instances.clear)

    def test_all_operations_and_worker_event_lifetime(self):
        main_thread = threading.get_ident()
        for name, symbol in (("ios_discover", "Discover"), ("ios_pair", "Pair"),
                             ("ios_check_connection", "CheckConnection"), ("ios_prepare", "Prepare")):
            events = []

            def event(value):
                events.append((threading.get_ident(), value))
                return True

            result = getattr(Toolkit, name)({"udid": "中文设备"}, event)
            if symbol == "Pair":
                result, credential = result
                self.assertEqual(credential, "opaque-test-credential")
                self.assertNotIn("credential", result)
            self.assertTrue(result["success"])
            self.assertEqual(result["label"], "中文")
            self.assertEqual(self.api.calls[-1], (f"MaaToolkitIOSDevice{symbol}", {"udid": "中文设备"}))
            self.assertEqual(events[0][1], {"type": "pin", "pin": "012345"})
            self.assertNotEqual(events[0][0], main_thread)

    def test_event_false_cancels_before_returning_result(self):
        result, credential = Toolkit.ios_pair(on_event=lambda event: False)
        self.assertEqual(credential, "")
        self.assertTrue(result["cancelled"])
        self.assertFalse(self.api.state["accepted"])

    def test_event_exception_cancels_and_is_raised_after_worker_cleanup(self):
        error = ValueError("event failed")

        def fail(event):
            raise error

        with self.assertRaises(ValueError) as caught:
            Toolkit.ios_pair(on_event=fail)
        self.assertIs(caught.exception, error)
        self.assertTrue(self.api.state["cancelled"])

    def test_cancel_exception_stops_native_worker_and_is_reraised(self):
        def fail():
            raise RuntimeError("cancel failed")

        with self.assertRaisesRegex(RuntimeError, "cancel failed"):
            Toolkit.ios_prepare(cancelled=fail)
        self.assertTrue(self.api.state["cancelled"])
        self.assertNotIn("accepted", self.api.state)

    def test_missing_wireless_event_handler_passes_null_for_native_pin_validation(self):
        Toolkit.ios_pair({"wifi": "auto"})
        self.assertFalse(self.api.state["has_event_callback"])

    def test_explicit_cancel_is_thread_safe(self):
        stop = threading.Event()
        stop.set()
        result = Toolkit.ios_check_connection(cancelled=stop.is_set)
        self.assertTrue(result["cancelled"])

    def test_invalid_options_do_not_enter_native_code(self):
        with self.assertRaises(TypeError):
            Toolkit.ios_discover([])
        self.assertEqual(self.api.calls, [])

    def test_native_signatures_match_callbacks_and_buffer_pointer(self):
        Toolkit._assign_api_properties()
        for name in ("Discover", "Pair", "CheckConnection", "Prepare"):
            function = getattr(self.api, f"MaaToolkitIOSDevice{name}")
            self.assertIs(function.restype, MaaBool)
            self.assertIs(function.argtypes[1], _IOSEventCallback)
            self.assertIs(function.argtypes[2], _IOSCancelCallback)
            self.assertIs(function.argtypes[4], MaaStringBufferHandle)
            if name == "Pair":
                self.assertIs(function.argtypes[5], MaaStringBufferHandle)


class NativeIOSManagementBoundaryTest(unittest.TestCase):
    def test_pair_credential_pointer_and_cancellation_without_device_io(self):
        status, credential = Toolkit.ios_pair(cancelled=lambda: True)
        if "disabled" in status.get("error", ""):
            self.skipTest("iOS support is disabled")
        self.assertFalse(status["success"])
        self.assertTrue(status["cancelled"])
        self.assertEqual(credential, "")


if __name__ == "__main__":
    unittest.main()
