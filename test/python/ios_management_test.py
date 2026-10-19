"""Offline tests for the native-worker / Python callback boundary. No device is accessed."""

import ctypes
import gc
import json
import threading
import unittest
from unittest.mock import MagicMock, patch

from maa.define import MaaBool, MaaStringBufferHandle, MaaIOSWiFiPairing, MaaIOSConnectionMethodEnum
from maa.ios import IOSWiFiPairing
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

    def __call__(self, *args):
        if self.name == "MaaToolkitIOSDeviceDestroyPairing":
            self.api.pairings.pop(ctypes.addressof(args[0].contents))
            return
        size = 2 if self.name.endswith("Pair") else (3 if self.name.endswith("Discover") else 4)
        inputs = args[:size]
        options, on_event, cancelled, userdata, result = args[size:size + 5]
        credential = args[size + 5] if self.name.endswith("Pair") else None
        self.api.inputs = inputs
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
            if not state["cancelled"]:
                native = IOSWiFiPairing("test-device", "test-host", "peer-设备", b"1" * 32, b"2" * 32, b"3" * 32)._native()
                self.api.pairings[ctypes.addressof(native)] = native
                ctypes.cast(credential, ctypes.POINTER(ctypes.POINTER(MaaIOSWiFiPairing)))[0] = ctypes.pointer(native)
        return not state["cancelled"]


class NativeAPI:
    def __init__(self):
        self.functions = {}
        self.pairings = {}
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

            result = getattr(Toolkit, name)({}, event, **({"udid": "device"} if symbol in ("CheckConnection", "Prepare") else {}))
            if symbol == "Pair":
                result, credential = result
                self.assertEqual(credential.udid, "test-device")
                self.assertEqual(credential.peer_identifier, "peer-设备")
                self.assertEqual(self.api.pairings, {})
                self.assertNotIn("credential", result)
            self.assertTrue(result["success"])
            self.assertEqual(result["label"], "中文")
            self.assertEqual(self.api.calls[-1], (f"MaaToolkitIOSDevice{symbol}", {}))
            self.assertEqual(events[0][1], {"type": "pin", "pin": "012345"})
            self.assertNotEqual(events[0][0], main_thread)

    def test_raw_credentials_and_connection_inputs(self):
        pairing = IOSWiFiPairing("device", "host", "peer", b"1" * 32, b"2" * 32, b"3" * 32)
        Toolkit.ios_discover(wifi_pairings=[pairing], connection_method=MaaIOSConnectionMethodEnum.WiFi)
        method, pointers, count = self.api.inputs
        self.assertEqual(method, MaaIOSConnectionMethodEnum.WiFi)
        self.assertEqual(count, 1)
        self.assertEqual(IOSWiFiPairing._from_native(pointers[0].contents), pairing)
        for method in (Toolkit.ios_check_connection, Toolkit.ios_prepare):
            method(udid="device", wifi_address="10.0.0.1", wifi_pairing=pairing,
                   connection_method=MaaIOSConnectionMethodEnum.WiFi)
            udid, connection, address, pointer = self.api.inputs
            self.assertEqual((udid, connection, address), (b"device", MaaIOSConnectionMethodEnum.WiFi, b"10.0.0.1"))
            self.assertEqual(IOSWiFiPairing._from_native(ctypes.cast(pointer, ctypes.POINTER(MaaIOSWiFiPairing)).contents), pairing)
        Toolkit.ios_pair(udid="device", connection_method=MaaIOSConnectionMethodEnum.USB)
        self.assertEqual(self.api.inputs, (b"device", MaaIOSConnectionMethodEnum.USB))

    def test_event_false_cancels_before_returning_result(self):
        result, credential = Toolkit.ios_pair(on_event=lambda event: False)
        self.assertIsNone(credential)
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
            Toolkit.ios_prepare(udid="device", cancelled=fail)
        self.assertTrue(self.api.state["cancelled"])
        self.assertNotIn("accepted", self.api.state)

    def test_missing_wireless_event_handler_passes_null_for_native_pin_validation(self):
        Toolkit.ios_pair(connection_method=MaaIOSConnectionMethodEnum.WiFi)
        self.assertFalse(self.api.state["has_event_callback"])

    def test_explicit_cancel_is_thread_safe(self):
        stop = threading.Event()
        stop.set()
        result = Toolkit.ios_check_connection(udid="device", cancelled=stop.is_set)
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
            offset = 2 if name == "Pair" else (3 if name == "Discover" else 4)
            self.assertIs(function.argtypes[offset + 1], _IOSEventCallback)
            self.assertIs(function.argtypes[offset + 2], _IOSCancelCallback)
            self.assertIs(function.argtypes[offset + 4], MaaStringBufferHandle)
            if name == "Pair":
                self.assertIs(function.argtypes[offset + 5], ctypes.POINTER(ctypes.POINTER(MaaIOSWiFiPairing)))


class IOSControllerCredentialTest(unittest.TestCase):
    def test_controller_passes_raw_credentials_and_explicit_transport(self):
        from maa.controller import Controller, IOSController
        pairing = IOSWiFiPairing("device", "host", "peer-设备", b"1" * 32, b"2" * 32, b"3" * 32)
        api = MagicMock()
        seen = []

        def create(udid, methods, connection, address, pointer):
            self.assertEqual(methods.value, (1 << 64) - 1)
            native = ctypes.cast(pointer, ctypes.POINTER(MaaIOSWiFiPairing)).contents
            seen.append((udid, connection.value, address, IOSWiFiPairing._from_native(native)))
            return 1

        api.MaaIOSControllerCreate.side_effect = create
        with patch.object(Library, "framework", return_value=api), patch.object(Controller, "__init__"):
            controller = IOSController(udid="device", connection_method=MaaIOSConnectionMethodEnum.WiFi, wifi_pairing=pairing)
            self.assertEqual(seen, [(b"device", MaaIOSConnectionMethodEnum.WiFi, None, pairing)])
            controller._handle = None


class NativeIOSManagementBoundaryTest(unittest.TestCase):
    def test_pairing_persistence_roundtrip_and_text_identity(self):
        for irk in (b"", bytes(16), b"4" * 16):
            pairing = IOSWiFiPairing("synthetic-device", "test-host", "peer-设备", b"1" * 32, b"2" * 32, b"3" * 32, irk)
            record = Toolkit.format_ios_pairing_record(pairing)
            self.assertTrue(record.startswith("MaaIOS1|"))
            self.assertNotIn("\n", record)
            self.assertIn("|pid=peer-设备|", record)
            restored = Toolkit.parse_ios_pairing_record(record)
            gc.collect()
            self.assertEqual(restored, pairing)
        with self.assertRaises(ValueError):
            Toolkit.parse_ios_pairing_record("invalid")
        with self.assertRaises(ValueError):
            Toolkit.parse_ios_pairing_record(record.replace("pid=", "pid=\0"))
        for identifier in ("invalid\0peer", "invalid\npeer"):
            pairing.peer_identifier = identifier
            with self.assertRaises(ValueError):
                Toolkit.format_ios_pairing_record(pairing)

    def test_pairing_identity_limits_are_shared_by_parse_format_and_controller(self):
        from maa.buffer import StringBuffer
        from maa.controller import IOSController
        pairing = IOSWiFiPairing("u" * 1024, "h" * 1024, "p" * 4096, b"1" * 32, b"2" * 32, b"3" * 32)
        record = Toolkit.format_ios_pairing_record(pairing)
        self.assertEqual(Toolkit.parse_ios_pairing_record(record), pairing)
        controller = IOSController(pairing.udid, connection_method=MaaIOSConnectionMethodEnum.WiFi, wifi_pairing=pairing)
        self.assertIsNotNone(controller._handle)
        for field, key, limit in (("udid", "udid", 1024), ("host_identifier", "hid", 1024), ("peer_identifier", "pid", 4096)):
            with self.subTest(field=field):
                oversized = "x" * (limit + 1)
                malformed = record.replace(f"|{key}={getattr(pairing, field)}|", f"|{key}={oversized}|", 1)
                with self.assertRaises(ValueError):
                    Toolkit.parse_ios_pairing_record(malformed)
                native = pairing._native()
                setattr(native, field, oversized.encode())
                output = StringBuffer()
                self.assertFalse(Library.toolkit().MaaToolkitIOSDeviceFormatPairingRecord(ctypes.byref(native), output._handle))
                self.assertFalse(Library.framework().MaaIOSControllerCreate(
                    native.udid, 2, MaaIOSConnectionMethodEnum.WiFi, None, ctypes.byref(native)))

    def test_pairing_text_delimiters_and_malformed_fields(self):
        pairing = IOSWiFiPairing("device", "host|%", "peer|%设备= ", b"1" * 32, b"2" * 32, b"3" * 32)
        record = Toolkit.format_ios_pairing_record(pairing)
        self.assertIn("|hid=host%7C%25|", record)
        self.assertIn("|pid=peer%7C%25设备= |", record)
        self.assertEqual(Toolkit.parse_ios_pairing_record(record), pairing)
        large = IOSWiFiPairing("%" * 1024, "|" * 1024, "%" * 4096, b"1" * 32, b"2" * 32, b"3" * 32)
        self.assertEqual(Toolkit.parse_ios_pairing_record(Toolkit.format_ios_pairing_record(large)), large)
        for invalid in (record + "|pid=duplicate", record + "|unknown=value", record + "|aid=value",
                        record + "|ruk=value", record + "|",
                        record + "\n", record.replace("MaaIOS1", "MaaIOS2", 1),
                        *(record.replace("host%7C%25", value) for value in ("%", "%GG", "%00", "%0A", "%0D"))):
            with self.subTest(invalid=invalid), self.assertRaises(ValueError):
                Toolkit.parse_ios_pairing_record(invalid)

    def test_controller_rejects_mismatched_pairing_before_device_io(self):
        from maa.controller import IOSController
        pairing = IOSWiFiPairing("synthetic-device", "test-host", "peer-设备", b"1" * 32, b"2" * 32, b"3" * 32)
        with self.assertRaises(RuntimeError):
            IOSController("different-device", connection_method=MaaIOSConnectionMethodEnum.WiFi, wifi_pairing=pairing)
        with self.assertRaises(RuntimeError):
            IOSController("synthetic-device", connection_method=MaaIOSConnectionMethodEnum.WiFi)
        with self.assertRaises(RuntimeError):
            IOSController("synthetic-device", connection_method=99)
        # Creation copies configuration but does not access the phone until post_connection.
        controller = IOSController("synthetic-device", connection_method=MaaIOSConnectionMethodEnum.USB, wifi_pairing=pairing)
        self.assertIsNotNone(controller._handle)
        api = Library.framework()
        native = pairing._native()
        for connection in MaaIOSConnectionMethodEnum:
            for udid in (None, b""):
                with self.subTest(connection=connection, udid=udid):
                    self.assertFalse(api.MaaIOSControllerCreate(
                        udid, 2, connection, None, ctypes.byref(native)))
            with self.assertRaises(ValueError):
                IOSController("", connection_method=connection, wifi_pairing=pairing)
            for operation in (Toolkit.ios_check_connection, Toolkit.ios_prepare):
                with self.assertRaises(ValueError):
                    operation(udid="", connection_method=connection, wifi_pairing=pairing,
                              cancelled=lambda: self.fail("Empty UDID reached cancellation callback"))

    def test_pair_credential_pointer_and_cancellation_without_device_io(self):
        status, credential = Toolkit.ios_pair(cancelled=lambda: True)
        if "disabled" in status.get("error", ""):
            self.skipTest("iOS support is disabled")
        self.assertFalse(status["success"])
        self.assertTrue(status["cancelled"])
        self.assertIsNone(credential)


if __name__ == "__main__":
    unittest.main()
