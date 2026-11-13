"""Device-free API contract tests against the built MaaFramework library."""

import json
from pathlib import Path
import struct
import tempfile
import threading
import traceback
import unittest
import zlib

import numpy

from maa.buffer import ImageBuffer, ImageListBuffer, RectBuffer, StringBuffer, StringListBuffer
from maa.context import ContextEventSink
from maa.controller import Controller, ControllerEventSink, CustomController
from maa.custom_action import CustomAction
from maa.custom_recognition import CustomRecognition
from maa.define import MaaStatusEnum, Rect
from maa.event_sink import NotificationType
from maa.pipeline import JActionType, JClick, JDirectHit, JDoNothing, JRecognitionType, JWaitFreezes
from maa.resource import Resource, ResourceEventSink
from maa.tasker import Tasker, TaskerEventSink


def write_png(path, image):
    rgb = image[:, :, ::-1]
    raw = b"".join(b"\x00" + row.tobytes() for row in rgb)

    def chunk(kind, data):
        return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data))

    header = struct.pack(">IIBBBBB", image.shape[1], image.shape[0], 8, 2, 0, 0, 0)
    path.write_bytes(
        b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", header) + chunk(b"IDAT", zlib.compress(raw)) + chunk(b"IEND", b"")
    )


class RecordingController(CustomController):
    """Return fixed pixels and record calls made through the native controller."""

    def __init__(self):
        self.calls = []
        self.fail = set()
        self.is_connected = False
        self.image = numpy.arange(24 * 32 * 3, dtype=numpy.uint8).reshape(24, 32, 3)
        super().__init__()
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


class RecordingResourceSink(ResourceEventSink):
    def __init__(self):
        self.events = []

    def on_resource_loading(self, resource, noti_type, detail):
        self.events.append((noti_type, detail))


class RecordingControllerSink(ControllerEventSink):
    def __init__(self):
        self.events = []

    def on_controller_action(self, controller, noti_type, detail):
        self.events.append((noti_type, detail))


class RecordingTaskerSink(TaskerEventSink):
    def __init__(self):
        self.events = []

    def on_raw_notification(self, tasker, msg, details):
        self.events.append((msg, details))


class RecordingContextSink(ContextEventSink):
    def __init__(self):
        self.events = []

    def on_raw_notification(self, context, msg, details):
        self.events.append((msg, details))


class CallbackAction(CustomAction):
    def __init__(self, callback):
        super().__init__()
        self.callback = callback
        self.calls = 0
        self.error = None

    def run(self, context, argv):
        self.calls += 1
        # ctypes callbacks cannot propagate assertions to the test thread.
        try:
            self.callback(context, argv)
        except Exception:
            self.error = traceback.format_exc()
            return False
        return True


class FixedRecognition(CustomRecognition):
    def __init__(self):
        super().__init__()
        self.args = []

    def analyze(self, context, argv):
        self.args.append(argv)
        return self.AnalyzeResult(box=(3, 4, 5, 6), detail={"text": "识别结果", "value": 42})


class ValueAction(CustomAction):
    def __init__(self, success):
        super().__init__()
        self.success = success
        self.args = []

    def run(self, context, argv):
        self.args.append(argv)
        return self.RunResult(success=self.success)


class BufferContractTest(unittest.TestCase):
    def test_string_round_trip_unicode_bytes_and_empty(self):
        buffer = StringBuffer()
        self.assertTrue(buffer.empty)
        self.assertEqual(buffer.get(), "")
        for value in ["MaaFramework", "中文 / 日本語 / 🐱", "bytes 字节".encode(), ""]:
            with self.subTest(value=value):
                self.assertTrue(buffer.set(value))
                expected = value.decode() if isinstance(value, bytes) else value
                self.assertEqual(buffer.get(), expected)
                self.assertEqual(buffer.empty, expected == "")
        self.assertTrue(buffer.set("clear me"))
        self.assertTrue(buffer.clear())
        self.assertEqual(buffer.get(), "")
        self.assertTrue(buffer.empty)

    def test_string_list_preserves_order_duplicates_and_replaces(self):
        buffer = StringListBuffer()
        self.assertEqual(buffer.get(), [])
        values = ["first", "中文", "", "first"]
        self.assertTrue(buffer.set(values))
        self.assertEqual(buffer.get(), values)
        self.assertTrue(buffer.append("last"))
        self.assertTrue(buffer.remove(1))
        self.assertEqual(buffer.get(), ["first", "", "first", "last"])
        self.assertTrue(buffer.set(["replacement"]))
        self.assertEqual(buffer.get(), ["replacement"])
        self.assertTrue(buffer.clear())
        self.assertEqual(buffer.get(), [])

    def test_string_list_invalid_remove_preserves_contents(self):
        buffer = StringListBuffer()
        self.assertFalse(buffer.remove(0))
        self.assertTrue(buffer.set(["one", "two"]))
        for index in [-1, 2, 100]:
            with self.subTest(index=index):
                self.assertFalse(buffer.remove(index))
                self.assertEqual(buffer.get(), ["one", "two"])

    def test_image_normalizes_supported_layouts(self):
        image = numpy.arange(8 * 10 * 3, dtype=numpy.uint8).reshape(8, 10, 3)
        gray = image[:, :, 0]
        cases = [
            (image, image),
            (gray, numpy.repeat(gray[:, :, None], 3, axis=2)),
            (gray[:, :, None], numpy.repeat(gray[:, :, None], 3, axis=2)),
            (numpy.concatenate([image, numpy.full((8, 10, 1), 123, dtype=numpy.uint8)], axis=2), image),
            (image[::-1, ::2], image[::-1, ::2]),
            (numpy.asfortranarray(image), image),
        ]
        buffer = ImageBuffer()
        for value, expected in cases:
            with self.subTest(shape=value.shape, strides=value.strides):
                self.assertTrue(buffer.set(value))
                self.assertFalse(buffer.empty)
                numpy.testing.assert_array_equal(buffer.get(), expected)

    def test_image_reads_and_writes_own_their_pixels(self):
        image = numpy.full((4, 6, 3), 42, dtype=numpy.uint8)
        buffer = ImageBuffer()
        self.assertTrue(buffer.set(image))
        image[:] = 1
        result = buffer.get()
        numpy.testing.assert_array_equal(result, numpy.full((4, 6, 3), 42, dtype=numpy.uint8))
        result[:] = 2
        numpy.testing.assert_array_equal(buffer.get(), numpy.full((4, 6, 3), 42, dtype=numpy.uint8))

    def test_image_rejects_invalid_types_and_shapes_without_replacing_pixels(self):
        buffer = ImageBuffer()
        expected = numpy.full((3, 5, 3), 17, dtype=numpy.uint8)
        self.assertTrue(buffer.set(expected))
        cases = [
            ([], TypeError),
            (numpy.zeros((3, 5, 3), dtype=numpy.float32), TypeError),
            (numpy.zeros((3, 5, 3), dtype=numpy.int32), TypeError),
            (numpy.zeros(5, dtype=numpy.uint8), ValueError),
            (numpy.zeros((1, 3, 5, 3), dtype=numpy.uint8), ValueError),
            (numpy.zeros((3, 5, 2), dtype=numpy.uint8), ValueError),
            (numpy.zeros((3, 5, 5), dtype=numpy.uint8), ValueError),
        ]
        for value, error in cases:
            with self.subTest(shape=getattr(value, "shape", None), dtype=getattr(value, "dtype", None)):
                with self.assertRaises(error):
                    buffer.set(value)
                numpy.testing.assert_array_equal(buffer.get(), expected)

    def test_image_resize_aspect_ratio_explicit_size_and_clear(self):
        buffer = ImageBuffer()
        self.assertTrue(buffer.empty)
        self.assertEqual(buffer.get().shape, (0, 0, 3))
        self.assertTrue(buffer.set(numpy.full((12, 20, 3), 75, dtype=numpy.uint8)))
        self.assertTrue(buffer.resize(width=10))
        self.assertEqual(buffer.get().shape, (6, 10, 3))
        self.assertTrue(buffer.resize(height=12))
        self.assertEqual(buffer.get().shape, (12, 20, 3))
        self.assertTrue(buffer.resize(width=5, height=4))
        self.assertEqual(buffer.get().shape, (4, 5, 3))
        self.assertTrue(buffer.clear())
        self.assertTrue(buffer.empty)
        self.assertEqual(buffer.get().shape, (0, 0, 3))

    def test_image_list_copies_replaces_removes_and_clears(self):
        buffer = ImageListBuffer()
        self.assertEqual(buffer.get(), [])
        first = numpy.full((4, 6, 3), 12, dtype=numpy.uint8)
        second = numpy.full((2, 3, 3), 34, dtype=numpy.uint8)
        self.assertTrue(buffer.set([first, second]))
        first[:] = 56
        numpy.testing.assert_array_equal(buffer.get()[0], numpy.full((4, 6, 3), 12, dtype=numpy.uint8))
        self.assertTrue(buffer.remove(0))
        numpy.testing.assert_array_equal(buffer.get()[0], second)
        self.assertFalse(buffer.remove(1))
        self.assertFalse(buffer.remove(-1))
        self.assertTrue(buffer.set([first]))
        self.assertEqual(len(buffer.get()), 1)
        numpy.testing.assert_array_equal(buffer.get()[0], first)
        self.assertTrue(buffer.clear())
        self.assertEqual(buffer.get(), [])

    def test_rect_supported_representations_and_signed_coordinates(self):
        buffer = RectBuffer()
        expected = Rect(-10, 20, 30, 40)
        for value in [expected, tuple(expected), list(expected), numpy.array(list(expected), dtype=numpy.int32)]:
            with self.subTest(kind=type(value).__name__):
                self.assertTrue(buffer.set(value))
                self.assertEqual(buffer.get(), expected)

    def test_rect_rejects_invalid_representations(self):
        buffer = RectBuffer()
        self.assertTrue(buffer.set((1, 2, 3, 4)))
        for value, error in [
            ((1, 2, 3), ValueError),
            ([1, 2, 3, 4, 5], ValueError),
            (numpy.zeros((2, 2), dtype=numpy.int32), ValueError),
            (numpy.zeros(4, dtype=numpy.int64), ValueError),
            ("1,2,3,4", TypeError),
        ]:
            with self.subTest(kind=type(value).__name__):
                with self.assertRaises(error):
                    buffer.set(value)
                self.assertEqual(buffer.get(), Rect(1, 2, 3, 4))


class ResourceContractTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="maafw-api-")
        self.addCleanup(self.temp.cleanup)
        self.bundle = Path(self.temp.name)
        self.pipeline = self.bundle / "pipeline"
        self.pipeline.mkdir()
        (self.pipeline / "nodes.json").write_text(
            json.dumps({"Entry": {"next": ["Finish"], "pre_delay": 0, "post_delay": 0}, "Finish": {"post_delay": 0}}),
            encoding="utf-8",
        )
        self.resource = Resource()

    def test_load_diagnostics_are_operation_scoped_and_visible_in_terminal_callback(self):
        snapshots = []

        class Sink(ResourceEventSink):
            def on_raw_notification(self, resource, message, details):
                if message.endswith((".Succeeded", ".Failed")):
                    snapshots.append((details, resource.get_load_detail(details["res_id"])))

        sink = Sink()
        self.resource.add_sink(sink)
        missing = self.resource.post_pipeline(self.bundle / "missing.json").wait()
        first = self.resource.get_load_detail(missing.job_id)
        self.assertTrue(missing.failed)
        self.assertEqual(first["errors"][0]["code"], "path_not_found")
        success = self.resource.post_bundle(self.bundle).wait()
        self.assertTrue(success.succeeded)
        self.assertEqual(self.resource.get_load_detail(success.job_id)["errors"], [])
        self.assertEqual(self.resource.get_load_detail(missing.job_id), first)
        self.assertEqual(len(snapshots), 2)
        for event, snapshot in snapshots:
            self.assertEqual(event, snapshot)
        self.assertIsNone(self.resource.get_load_detail(0))
        self.resource.clear()
        self.assertIsNone(self.resource.get_load_detail(missing.job_id))

    def test_load_outcome_survives_enqueue_from_callbacks(self):
        for phase in ("Starting", "Succeeded"):
            with self.subTest(phase=phase):
                queued = []
                resource = Resource()
                source = self.pipeline / "nodes.json"

                class Sink(ResourceEventSink):
                    def on_raw_notification(self, current, message, details):
                        if message == "Resource.Loading." + phase and not queued:
                            queued.append(current.post_pipeline(source))

                sink = Sink()
                resource.add_sink(sink)
                first = resource.post_pipeline(source).wait()
                self.assertEqual(len(queued), 1)
                second = queued[0].wait()
                for job in (first, second):
                    self.assertTrue(job.succeeded)
                    detail = resource.get_load_detail(job.job_id)
                    self.assertEqual(detail["status"], MaaStatusEnum.succeeded)
                    self.assertEqual(detail["errors"], [])

    def test_load_diagnostics_do_not_attribute_runtime_overrides_to_files(self):
        self.assertTrue(self.resource.post_bundle(self.bundle).wait().succeeded)
        self.assertTrue(self.resource.override_next("Entry", ["Missing"]))
        addition = self.bundle / "addition.json"
        addition.write_text('{"Other": {}}', encoding="utf-8")
        job = self.resource.post_pipeline(addition).wait()
        self.assertTrue(job.failed)
        error = self.resource.get_load_detail(job.job_id)["errors"][0]
        self.assertEqual(error["target"], "Missing")
        self.assertNotIn("file", error)
        self.assertNotIn("pointer", error)

    def test_load_diagnostics_locate_pipeline_errors(self):
        source = self.pipeline / "nodes.json"
        cases = [
            ("{", "json_read_failed", None),
            ("[]", "invalid_pipeline_root", None),
            ('{"Bad": 1}', "invalid_node", "Bad"),
            ('{"A~/B": {"next": ["Missing"]}}', "node_reference_not_found", "A~/B"),
            ('{"A": {"on_error": ["Missing"]}}', "node_reference_not_found", "A"),
        ]
        for payload, code, node in cases:
            with self.subTest(code=code, payload=payload):
                self.resource.clear()
                source.write_text(payload, encoding="utf-8")
                job = self.resource.post_bundle(self.bundle).wait()
                self.assertTrue(job.failed)
                error = self.resource.get_load_detail(job.job_id)["errors"][0]
                self.assertEqual(error["code"], code)
                self.assertEqual(Path(error["file"]), source)
                if node:
                    self.assertEqual(error["node"], node)
                if code == "node_reference_not_found":
                    self.assertEqual(error["target"], "Missing")
                    expected = "/A~0~1B/next" if node == "A~/B" else "/A/on_error"
                    self.assertEqual(error["pointer"], expected)

    def test_bundle_job_state_and_exact_node_readback(self):
        job = self.resource.post_bundle(self.bundle)
        self.assertIs(job.wait(), job)
        self.assertGreater(job.job_id, 0)
        self.assertTrue(job.done)
        self.assertTrue(job.succeeded)
        self.assertFalse(job.failed)
        self.assertFalse(job.pending)
        self.assertFalse(job.running)
        self.assertTrue(self.resource.loaded)
        self.assertEqual(set(self.resource.node_list), {"Entry", "Finish"})
        self.assertIsNotNone(self.resource.get_node_object("Entry"))
        self.assertIsNone(self.resource.get_node_data("Missing"))
        self.assertIsNone(self.resource.get_node_object("Missing"))
        self.assertTrue(self.resource.hash)
        self.assertEqual(self.resource.hash, self.resource.hash)

    def test_failed_load_reports_failure_and_valid_load_recovers(self):
        job = self.resource.post_bundle(self.bundle / "missing").wait()
        self.assertTrue(job.done)
        self.assertTrue(job.failed)
        self.assertFalse(job.succeeded)
        self.assertFalse(self.resource.loaded)
        self.assertTrue(self.resource.post_bundle(self.bundle).wait().succeeded)
        self.assertTrue(self.resource.loaded)
        self.assertEqual(set(self.resource.node_list), {"Entry", "Finish"})

    def test_post_pipeline_loads_without_a_bundle(self):
        job = self.resource.post_pipeline(self.pipeline).wait()
        self.assertTrue(job.succeeded)
        self.assertTrue(self.resource.loaded)
        self.assertEqual(set(self.resource.node_list), {"Entry", "Finish"})

    def test_post_pipeline_accepts_a_single_file(self):
        self.assertTrue(self.resource.post_pipeline(self.pipeline / "nodes.json").wait().succeeded)
        self.assertEqual(set(self.resource.node_list), {"Entry", "Finish"})

    def test_partial_loaders_report_missing_paths_and_recover(self):
        for method in ["post_pipeline", "post_image", "post_ocr_model"]:
            with self.subTest(method=method):
                resource = Resource()
                job = getattr(resource, method)(self.bundle / "missing").wait()
                self.assertTrue(job.failed)
                self.assertFalse(resource.loaded)
                self.assertTrue(resource.post_bundle(self.bundle).wait().succeeded)
                self.assertTrue(resource.loaded)

    def test_post_image_file_is_available_to_template_recognition(self):
        image = numpy.random.default_rng(2026).integers(0, 256, (24, 32, 3), dtype=numpy.uint8)
        image_path = self.bundle / "template.png"
        write_png(image_path, image)
        self.assertTrue(self.resource.post_image(image_path).wait().succeeded)
        controller = RecordingController()
        controller.image = image
        self.assertTrue(controller.post_connection().wait().succeeded)
        tasker = Tasker()
        self.assertTrue(tasker.bind(self.resource, controller))
        job = tasker.post_task(
            "FindTemplate",
            {
                "FindTemplate": {
                    "recognition": "TemplateMatch",
                    "template": "template.png",
                    "threshold": 0.99,
                    "pre_delay": 0,
                    "post_delay": 0,
                }
            },
        ).wait()
        self.assertTrue(job.succeeded)
        recognition = job.get().nodes[0].recognition
        self.assertTrue(recognition.hit)
        self.assertEqual(recognition.box, Rect(0, 0, 32, 24))

    def test_bundle_overlay_preserves_existing_nodes_and_updates_fields(self):
        self.assertTrue(self.resource.post_bundle(self.bundle).wait().succeeded)
        overlay = self.bundle / "overlay"
        (overlay / "pipeline").mkdir(parents=True)
        (overlay / "pipeline" / "nodes.json").write_text(
            json.dumps({"Entry": {"post_delay": 17}, "Added": {}}), encoding="utf-8"
        )
        self.assertTrue(self.resource.post_bundle(overlay).wait().succeeded)
        self.assertEqual(set(self.resource.node_list), {"Entry", "Finish", "Added"})
        self.assertEqual(self.resource.get_node_object("Entry").post_delay, 17)

    def test_clear_discards_nodes_and_hash_then_allows_reloading(self):
        self.assertTrue(self.resource.post_bundle(self.bundle).wait().succeeded)
        self.assertTrue(self.resource.clear())
        self.assertEqual(self.resource.node_list, [])
        self.assertIsNone(self.resource.get_node_data("Entry"))
        with self.assertRaises(RuntimeError):
            _ = self.resource.hash
        self.assertTrue(self.resource.post_bundle(self.bundle).wait().succeeded)
        self.assertEqual(set(self.resource.node_list), {"Entry", "Finish"})

    def test_loading_sink_correlates_success_failure_and_removal(self):
        sink = RecordingResourceSink()
        sink_id = self.resource.add_sink(sink)
        self.assertIsNotNone(sink_id)
        success = self.resource.post_bundle(self.bundle).wait()
        failure = self.resource.post_bundle(self.bundle / "missing").wait()
        for job, expected in [(success, NotificationType.Succeeded), (failure, NotificationType.Failed)]:
            events = [(kind, detail) for kind, detail in sink.events if detail.res_id == job.job_id]
            self.assertEqual([kind for kind, _ in events], [NotificationType.Starting, expected])
            self.assertTrue(all(detail.type == "Bundle" for _, detail in events))
        self.resource.remove_sink(sink_id)
        count = len(sink.events)
        self.assertTrue(self.resource.post_bundle(self.bundle).wait().succeeded)
        self.assertEqual(len(sink.events), count)

    def test_clear_sinks_stops_all_loading_notifications(self):
        sinks = [RecordingResourceSink(), RecordingResourceSink()]
        for sink in sinks:
            self.assertIsNotNone(self.resource.add_sink(sink))
        self.assertTrue(self.resource.post_bundle(self.bundle).wait().succeeded)
        self.assertEqual([len(sink.events) for sink in sinks], [2, 2])
        self.resource.clear_sinks()
        self.assertTrue(self.resource.post_bundle(self.bundle).wait().succeeded)
        self.assertEqual([len(sink.events) for sink in sinks], [2, 2])


class ControllerContractTest(unittest.TestCase):
    def setUp(self):
        self.controller = RecordingController()
        self.assertTrue(self.controller.post_connection().wait().succeeded)
        self.assertTrue(self.controller.post_screencap().wait().succeeded)
        self.controller.calls.clear()

    def test_identity_info_and_resolution(self):
        borrowed = Controller(self.controller._handle)
        self.assertTrue(borrowed.connected)
        self.assertEqual(self.controller.uuid, "api-contract-controller")
        self.assertEqual(self.controller.info["type"], "custom")
        self.assertEqual(self.controller.info["fixture"], "接口测试")
        self.assertEqual(self.controller.info["nested"], {"enabled": True})
        self.assertEqual(self.controller.resolution, (32, 24))

    def test_failed_connection_reports_state_and_allows_retry(self):
        controller = RecordingController()
        controller.fail.add("connect")
        self.assertTrue(controller.post_connection().wait().failed)
        self.assertFalse(Controller(controller._handle).connected)
        controller.fail.clear()
        self.assertTrue(controller.post_connection().wait().succeeded)
        self.assertTrue(Controller(controller._handle).connected)

    def test_input_calls_preserve_arguments(self):
        cases = [
            ("post_click", (4, 6), ("click", 4, 6)),
            ("post_swipe", (2, 3, 14, 16, 20), ("swipe", 2, 3, 14, 16, 20)),
            ("post_touch_down", (8, 9, 2, 7), ("touch_down", 2, 8, 9, 7)),
            ("post_touch_move", (10, 11, 2, 6), ("touch_move", 2, 10, 11, 6)),
            ("post_touch_up", (2,), ("touch_up", 2)),
            ("post_click_key", (65,), ("click_key", 65)),
            ("post_key_down", (66,), ("key_down", 66)),
            ("post_key_up", (66,), ("key_up", 66)),
            ("post_input_text", ("测试 UTF-8 🐱",), ("input_text", "测试 UTF-8 🐱")),
            ("post_start_app", ("test.package/.Main",), ("start_app", "test.package/.Main")),
            ("post_stop_app", ("test.package",), ("stop_app", "test.package")),
            ("post_scroll", (-120, 240), ("scroll", -120, 240)),
            ("post_relative_move", (-3, 4), ("relative_move", -3, 4)),
            ("post_shell", ("echo 中文🐱", 1357), ("shell", "echo 中文🐱", 1357)),
            ("post_inactive", (), ("inactive",)),
        ]
        for method, args, expected in cases:
            with self.subTest(method=method):
                self.controller.calls.clear()
                job = getattr(self.controller, method)(*args)
                self.assertIs(job.wait(), job)
                self.assertTrue(job.succeeded)
                self.assertTrue(job.done)
                self.assertFalse(job.failed)
                self.assertIn(expected, self.controller.calls)
                if method == "post_shell":
                    self.assertEqual(job.get(), "stdout:echo 中文🐱\n结果🐱")
                    self.assertEqual(self.controller.shell_output, "stdout:echo 中文🐱\n结果🐱")

    def test_failed_input_job_recovers_on_next_operation(self):
        self.controller.fail.add("input_text")
        job = self.controller.post_input_text("rejected").wait()
        self.assertTrue(job.failed)
        self.assertTrue(job.done)
        self.assertFalse(job.succeeded)
        self.controller.fail.clear()
        self.assertTrue(self.controller.post_input_text("accepted").wait().succeeded)
        self.assertIn(("input_text", "accepted"), self.controller.calls)
        self.controller.fail.add("shell")
        shell_job = self.controller.post_shell("rejected", 2468).wait()
        self.assertTrue(shell_job.failed)
        self.assertTrue(shell_job.done)
        self.assertFalse(shell_job.succeeded)
        self.assertIn(("shell", "rejected", 2468), self.controller.calls)
        self.controller.fail.clear()
        recovered = self.controller.post_shell("accepted", 3579).wait()
        self.assertTrue(recovered.succeeded)
        self.assertEqual(recovered.get(), "stdout:accepted\n结果🐱")
        self.assertIn(("shell", "accepted", 3579), self.controller.calls)

    def test_screencap_and_cached_image_are_pixel_exact_copies(self):
        job = self.controller.post_screencap()
        image = job.get(wait=True)
        self.assertTrue(job.succeeded)
        numpy.testing.assert_array_equal(image, self.controller.image)
        numpy.testing.assert_array_equal(self.controller.cached_image, self.controller.image)
        image[:] = 0
        cached = self.controller.cached_image
        cached[:] = 0
        numpy.testing.assert_array_equal(self.controller.cached_image, self.controller.image)

    def test_screenshot_scaling_keeps_original_resolution(self):
        self.assertTrue(self.controller.set_screenshot_use_raw_size(False))
        self.assertTrue(self.controller.set_screenshot_target_long_side(64))
        image = self.controller.post_screencap().get(wait=True)
        self.assertEqual(image.shape, (48, 64, 3))
        self.assertEqual(self.controller.resolution, (32, 24))
        self.assertTrue(self.controller.set_screenshot_use_raw_size(True))
        self.assertEqual(self.controller.post_screencap().get(wait=True).shape, (24, 32, 3))

    def test_controller_sink_correlates_success_failure_and_removal(self):
        sink = RecordingControllerSink()
        sink_id = self.controller.add_sink(sink)
        self.assertIsNotNone(sink_id)
        success = self.controller.post_input_text("ok").wait()
        self.controller.fail.add("input_text")
        failure = self.controller.post_input_text("failed").wait()
        for job, expected in [(success, NotificationType.Succeeded), (failure, NotificationType.Failed)]:
            events = [(kind, detail) for kind, detail in sink.events if detail.ctrl_id == job.job_id]
            self.assertEqual([kind for kind, _ in events], [NotificationType.Starting, expected])
            self.assertTrue(all(detail.uuid == "api-contract-controller" for _, detail in events))
        self.controller.remove_sink(sink_id)
        count = len(sink.events)
        self.controller.fail.clear()
        self.assertTrue(self.controller.post_input_text("removed").wait().succeeded)
        self.assertEqual(len(sink.events), count)

    def test_clear_controller_sinks_stops_all_notifications(self):
        sinks = [RecordingControllerSink(), RecordingControllerSink()]
        for sink in sinks:
            self.assertIsNotNone(self.controller.add_sink(sink))
        self.assertTrue(self.controller.post_input_text("before clear").wait().succeeded)
        self.assertEqual([len(sink.events) for sink in sinks], [2, 2])
        self.controller.clear_sinks()
        self.assertTrue(self.controller.post_input_text("after clear").wait().succeeded)
        self.assertEqual([len(sink.events) for sink in sinks], [2, 2])


class TaskerContractTest(unittest.TestCase):
    def setUp(self):
        self.resource = Resource()
        self.assertTrue(
            self.resource.override_pipeline(
                {
                    "Entry": {"next": ["Finish"], "pre_delay": 0, "post_delay": 0},
                    "Finish": {"pre_delay": 0, "post_delay": 0},
                }
            )
        )
        self.controller = RecordingController()
        self.assertTrue(self.controller.post_connection().wait().succeeded)
        self.tasker = Tasker()
        self.assertTrue(self.tasker.bind(self.resource, self.controller))
        self.assertTrue(self.tasker.inited)

    def tearDown(self):
        self.tasker.post_stop().wait()

    def test_unbound_tasker_reports_not_initialized_and_missing_instances(self):
        tasker = Tasker()
        self.assertFalse(tasker.inited)
        with self.assertRaises(RuntimeError):
            _ = tasker.resource
        with self.assertRaises(RuntimeError):
            _ = tasker.controller

    def test_bound_instances_are_live_views(self):
        self.assertTrue(self.tasker.resource.override_pipeline({"FromView": {}}))
        self.assertIn("FromView", self.resource.node_list)
        self.assertEqual(self.tasker.controller.uuid, self.controller.uuid)
        self.assertTrue(self.tasker.controller.post_input_text("view").wait().succeeded)
        self.assertIn(("input_text", "view"), self.controller.calls)

    def test_task_details_nodes_recognition_and_actions_correlate(self):
        job = self.tasker.post_task("Entry")
        detail = job.get(wait=True)
        self.assertTrue(job.succeeded)
        self.assertTrue(detail.status.succeeded)
        self.assertEqual(detail.task_id, job.job_id)
        self.assertEqual(detail.entry, "Entry")
        self.assertEqual([node.name for node in detail.nodes], ["Entry", "Finish"])
        self.assertEqual(detail.node_id_list, [node.node_id for node in detail.nodes])
        self.assertIs(detail.nodes, detail.nodes)
        for node in detail.nodes:
            with self.subTest(node=node.name):
                self.assertTrue(node.completed)
                self.assertEqual(self.tasker.get_latest_node(node.name).node_id, node.node_id)
                self.assertEqual(self.tasker.get_node_detail(node.node_id).name, node.name)
                self.assertTrue(node.recognition.hit)
                self.assertEqual(node.recognition.algorithm, JRecognitionType.DirectHit)
                self.assertEqual(self.tasker.get_recognition_detail(node.recognition.reco_id).name, node.name)
                self.assertTrue(node.action.success)
                self.assertEqual(node.action.action, JActionType.DoNothing)
                self.assertEqual(self.tasker.get_action_detail(node.action.action_id).name, node.name)

    def test_unknown_details_return_none(self):
        for method in [
            "get_task_detail",
            "get_node_detail",
            "get_recognition_detail",
            "get_action_detail",
            "get_wait_freezes_detail",
        ]:
            with self.subTest(method=method):
                self.assertIsNone(getattr(self.tasker, method)(0))
        self.assertIsNone(self.tasker.get_latest_node("Missing"))
        self.assertFalse(self.tasker.override_pipeline(0, {"Entry": {"post_delay": 1}}))

    def test_pipeline_override_is_local_to_one_task(self):
        first = self.tasker.post_task("Entry", {"Entry": {"next": []}}).get(wait=True)
        second = self.tasker.post_task("Entry").get(wait=True)
        self.assertEqual([node.name for node in first.nodes], ["Entry"])
        self.assertEqual([node.name for node in second.nodes], ["Entry", "Finish"])
        self.assertNotEqual(first.task_id, second.task_id)

    def test_running_job_override_changes_its_next_nodes_only(self):
        entered = threading.Event()
        release = threading.Event()

        def gate(context, argv):
            entered.set()
            self.assertTrue(release.wait(10), "test did not release the custom action")

        action = CallbackAction(gate)
        self.assertTrue(self.resource.register_custom_action("Gate", action))
        self.assertTrue(self.resource.override_pipeline({"Entry": {"action": "Custom", "custom_action": "Gate"}}))
        job = self.tasker.post_task("Entry")
        try:
            self.assertTrue(entered.wait(10), "custom action did not start")
            self.assertTrue(job.running)
            self.assertTrue(self.tasker.running)
            self.assertTrue(job.override_pipeline({"Entry": {"next": []}}))
        finally:
            release.set()
            job.wait()
        self.assertIsNone(action.error, action.error)
        self.assertTrue(job.succeeded)
        self.assertEqual([node.name for node in job.get().nodes], ["Entry"])
        normal = self.tasker.post_task("Entry", {"Entry": {"action": "DoNothing"}}).get(wait=True)
        self.assertEqual([node.name for node in normal.nodes], ["Entry", "Finish"])

    def test_failed_action_preserves_failure_details_and_next_task_recovers(self):
        self.controller.fail.add("input_text")
        job = self.tasker.post_task(
            "Entry", {"Entry": {"action": "InputText", "input_text": "rejected", "next": []}}
        ).wait()
        self.assertTrue(job.failed)
        detail = job.get()
        self.assertTrue(detail.status.failed)
        self.assertEqual(len(detail.nodes), 1)
        action = detail.nodes[0].action
        self.assertIsNotNone(action)
        self.assertFalse(action.success)
        self.assertGreater(action.action_id, 0)
        self.assertEqual(action.action, JActionType.InputText)
        self.controller.fail.clear()
        self.assertTrue(self.tasker.post_task("Entry").wait().succeeded)

    def test_clear_cache_removes_all_queryable_details_then_task_runs_again(self):
        job = self.tasker.post_task("Entry").wait()
        detail = job.get()
        node = detail.nodes[0]
        self.assertTrue(self.tasker.clear_cache())
        self.assertIsNone(self.tasker.get_task_detail(detail.task_id))
        self.assertIsNone(self.tasker.get_node_detail(node.node_id))
        self.assertIsNone(self.tasker.get_recognition_detail(node.recognition.reco_id))
        self.assertIsNone(self.tasker.get_action_detail(node.action.action_id))
        self.assertIsNone(self.tasker.get_latest_node("Entry"))
        self.assertTrue(self.tasker.post_task("Entry").wait().succeeded)

    def test_direct_action_reports_exact_result_and_controller_call(self):
        detail = self.tasker.post_action(JActionType.Click, JClick(target=[7, 9, 1, 1])).get(wait=True)
        self.assertTrue(detail.status.succeeded)
        self.assertEqual(len(detail.nodes), 1)
        action = detail.nodes[0].action
        self.assertTrue(action.success)
        self.assertEqual(action.action, JActionType.Click)
        self.assertEqual(tuple(action.result.point), (7, 9))
        self.assertIn(("click", 7, 9), self.controller.calls)

    def test_direct_recognition_uses_supplied_image(self):
        detail = self.tasker.post_recognition(JRecognitionType.DirectHit, JDirectHit(), self.controller.image).get(
            wait=True
        )
        self.assertTrue(detail.status.succeeded)
        self.assertEqual(len(detail.nodes), 1)
        recognition = detail.nodes[0].recognition
        self.assertTrue(recognition.hit)
        self.assertEqual(recognition.algorithm, JRecognitionType.DirectHit)

    def test_recognition_node_uses_pixels_without_actions_or_successors(self):
        image = numpy.zeros((20, 30, 3), dtype=numpy.uint8)
        self.controller.calls.clear()
        detail = self.tasker.post_recognition_node(
            "Entry", image,
            {"Entry": {"recognition": "ColorMatch", "lower": [0, 0, 0], "upper": [0, 0, 0],
                       "action": "Click", "target": [7, 9, 1, 1], "on_error": ["Finish"]}},
        ).get(wait=True)
        self.assertTrue(detail.status.succeeded)
        self.assertEqual([node.name for node in detail.nodes], ["Entry"])
        self.assertTrue(detail.nodes[0].completed)
        self.assertTrue(detail.nodes[0].recognition.hit)
        self.assertIsNone(detail.nodes[0].action)
        self.assertFalse(any(call[0] in {"click", "screencap"} for call in self.controller.calls))
        self.assertEqual(self.resource.get_node_data("Entry")["recognition"]["type"], "DirectHit")

    def test_recognition_node_reports_effective_hit_after_inverse(self):
        image = numpy.zeros((20, 30, 3), dtype=numpy.uint8)
        for inverse, expected in ((False, False), (True, True)):
            with self.subTest(inverse=inverse):
                detail = self.tasker.post_recognition_node(
                    "Entry", image,
                    {"Entry": {"recognition": "ColorMatch", "lower": [255, 255, 255],
                               "upper": [255, 255, 255], "inverse": inverse}},
                ).get(wait=True)
                self.assertTrue(detail.status.succeeded)
                self.assertEqual(len(detail.nodes), 1)
                self.assertEqual(detail.nodes[0].completed, expected)
                self.assertFalse(detail.nodes[0].recognition.hit)
                self.assertIsNone(detail.nodes[0].action)

    def test_recognition_node_supports_composed_and_custom_recognition(self):
        recognition = FixedRecognition()
        self.assertTrue(self.resource.register_custom_recognition("Fixed", recognition))
        for kind, field in (("And", "all_of"), ("Or", "any_of")):
            with self.subTest(kind=kind):
                detail = self.tasker.post_recognition_node(
                    "Entry", self.controller.image,
                    {"Entry": {"recognition": {"type": kind, "param": {field: ["ProbeCustom"]}}},
                     "ProbeCustom": {"recognition": "Custom", "custom_recognition": "Fixed"}},
                ).get(wait=True)
                self.assertTrue(detail.status.succeeded)
                self.assertTrue(detail.nodes[0].completed)
                self.assertEqual(detail.nodes[0].recognition.algorithm, kind)
                self.assertEqual(len(detail.nodes), 1)
        self.assertEqual(len(recognition.args), 2)
        self.assertIsNone(self.resource.get_node_data("ProbeCustom"))

    def test_recognition_node_missing_entry_fails_without_running_successors(self):
        detail = self.tasker.post_recognition_node("MissingProbe", self.controller.image).get(wait=True)
        self.assertTrue(detail.status.failed)
        self.assertEqual(detail.nodes, [])

    def test_custom_recognition_forwards_parameters_and_result_details(self):
        recognition = FixedRecognition()
        self.assertTrue(self.resource.register_custom_recognition("Fixed", recognition))
        job = self.tasker.post_task(
            "Entry",
            {
                "Entry": {
                    "recognition": "Custom",
                    "custom_recognition": "Fixed",
                    "custom_recognition_param": {"language": "中文", "number": 7},
                    "roi": [1, 2, 10, 12],
                    "next": [],
                }
            },
        ).wait()
        self.assertTrue(job.succeeded)
        self.assertEqual(len(recognition.args), 1)
        argv = recognition.args[0]
        self.assertEqual(argv.node_name, "Entry")
        self.assertEqual(argv.custom_recognition_name, "Fixed")
        self.assertEqual(json.loads(argv.custom_recognition_param), {"language": "中文", "number": 7})
        self.assertEqual(argv.roi, Rect(1, 2, 10, 12))
        numpy.testing.assert_array_equal(argv.image, self.controller.image)
        result = job.get().nodes[0].recognition
        self.assertTrue(result.hit)
        self.assertEqual(result.box, Rect(3, 4, 5, 6))
        self.assertEqual(result.best_result.detail, {"text": "识别结果", "value": 42})

    def test_custom_action_forwards_parameters_and_success_or_failure(self):
        self.assertTrue(self.resource.register_custom_recognition("Fixed", FixedRecognition()))
        for success in [True, False]:
            with self.subTest(success=success):
                action = ValueAction(success)
                name = f"ValueAction-{success}"
                self.assertTrue(self.resource.register_custom_action(name, action))
                job = self.tasker.post_task(
                    "Entry",
                    {
                        "Entry": {
                            "recognition": "Custom",
                            "custom_recognition": "Fixed",
                            "action": "Custom",
                            "custom_action": name,
                            "custom_action_param": {"text": "动作参数", "success": success},
                            "next": [],
                        }
                    },
                ).wait()
                self.assertEqual(job.succeeded, success)
                self.assertEqual(job.failed, not success)
                self.assertEqual(len(action.args), 1)
                argv = action.args[0]
                self.assertEqual(argv.task_detail.task_id, job.job_id)
                self.assertEqual(argv.node_name, "Entry")
                self.assertEqual(argv.custom_action_name, name)
                self.assertEqual(json.loads(argv.custom_action_param), {"text": "动作参数", "success": success})
                self.assertEqual(argv.box, Rect(3, 4, 5, 6))
                self.assertTrue(argv.reco_detail.hit)
                self.assertEqual(argv.reco_detail.best_result.detail, {"text": "识别结果", "value": 42})
                self.assertEqual(job.get().nodes[0].action.success, success)
                self.assertTrue(self.resource.unregister_custom_action(name))
                self.assertNotIn(name, self.resource.custom_action_list)

    def test_task_and_context_sink_correlate_ids_then_remove(self):
        task_sink = RecordingTaskerSink()
        context_sink = RecordingContextSink()
        task_sink_id = self.tasker.add_sink(task_sink)
        context_sink_id = self.tasker.add_context_sink(context_sink)
        self.assertIsNotNone(task_sink_id)
        self.assertIsNotNone(context_sink_id)
        job = self.tasker.post_task("Entry").wait()
        events = [(msg, data) for msg, data in task_sink.events if data.get("task_id") == job.job_id]
        self.assertEqual([msg for msg, _ in events], ["Tasker.Task.Starting", "Tasker.Task.Succeeded"])
        self.assertTrue(context_sink.events)
        self.assertTrue(all(data["task_id"] == job.job_id for _, data in context_sink.events))
        self.tasker.remove_sink(task_sink_id)
        self.tasker.remove_context_sink(context_sink_id)
        counts = (len(task_sink.events), len(context_sink.events))
        self.assertTrue(self.tasker.post_task("Entry").wait().succeeded)
        self.assertEqual((len(task_sink.events), len(context_sink.events)), counts)

    def test_clear_tasker_and_context_sinks_stop_notifications(self):
        task_sink = RecordingTaskerSink()
        context_sink = RecordingContextSink()
        self.assertIsNotNone(self.tasker.add_sink(task_sink))
        self.assertIsNotNone(self.tasker.add_context_sink(context_sink))
        self.assertTrue(self.tasker.post_task("Entry").wait().succeeded)
        self.assertEqual(len(task_sink.events), 2)
        self.assertTrue(context_sink.events)
        self.tasker.clear_sinks()
        self.tasker.clear_context_sinks()
        counts = (len(task_sink.events), len(context_sink.events))
        self.assertTrue(self.tasker.post_task("Entry").wait().succeeded)
        self.assertEqual((len(task_sink.events), len(context_sink.events)), counts)

    def run_context_probe(self, callback):
        action = CallbackAction(callback)
        self.assertTrue(self.resource.register_custom_action("Probe", action))
        job = self.tasker.post_task(
            "ProbeNode", {"ProbeNode": {"action": "Custom", "custom_action": "Probe", "pre_delay": 0, "post_delay": 0}}
        ).wait()
        self.assertEqual(action.calls, 1)
        self.assertIsNone(action.error, action.error)
        self.assertTrue(job.succeeded)

    def test_context_clone_override_isolation_and_shared_anchors(self):
        def probe(context, argv):
            clone = context.clone()
            self.assertTrue(clone.override_pipeline({"Entry": {"post_delay": 23}, "CloneOnly": {}}))
            self.assertEqual(clone.get_node_object("Entry").post_delay, 23)
            self.assertEqual(context.get_node_object("Entry").post_delay, 0)
            self.assertIsNone(context.get_node_data("CloneOnly"))
            self.assertIsNone(self.resource.get_node_data("CloneOnly"))
            self.assertIsNone(context.get_anchor("checkpoint"))
            self.assertTrue(clone.set_anchor("checkpoint", "Finish"))
            self.assertEqual(context.get_anchor("checkpoint"), "Finish")
            self.assertTrue(context.set_anchor("checkpoint", ""))
            self.assertIsNone(clone.get_anchor("checkpoint"))
            self.assertEqual(context.get_task_job().job_id, argv.task_detail.task_id)
            self.assertEqual(context.tasker.controller.uuid, self.controller.uuid)

        self.run_context_probe(probe)

    def test_context_nested_task_hits_and_direct_execution(self):
        def probe(context, argv):
            self.assertEqual(context.get_hit_count("Entry"), 0)
            detail = context.run_task("Entry")
            self.assertTrue(detail.status.succeeded)
            self.assertEqual([node.name for node in detail.nodes], ["Entry", "Finish"])
            self.assertEqual(context.get_hit_count("Entry"), 1)
            clone = context.clone()
            self.assertEqual(clone.get_hit_count("Entry"), 1)
            self.assertTrue(clone.clear_hit_count("Entry"))
            self.assertEqual(context.get_hit_count("Entry"), 0)
            recognition = context.run_recognition_direct(
                JRecognitionType.DirectHit, JDirectHit(), self.controller.image
            )
            self.assertIsNotNone(recognition)
            self.assertTrue(recognition.hit)
            action = context.run_action_direct(JActionType.DoNothing, JDoNothing())
            self.assertIsNotNone(action)
            self.assertTrue(action.success)

        self.run_context_probe(probe)

    def test_context_wait_freezes_rejects_missing_or_conflicting_time(self):
        def probe(context, argv):
            self.assertFalse(context.wait_freezes(time=0, wait_freezes_param=JWaitFreezes(time=0)))
            self.assertFalse(context.wait_freezes(time=1, wait_freezes_param=JWaitFreezes(time=1)))

        self.run_context_probe(probe)


if __name__ == "__main__":
    unittest.main()
