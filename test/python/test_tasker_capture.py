"""Global evidence policy and execution-time configuration regression tests."""
import ctypes
import json
from pathlib import Path
import tempfile
import unittest

import numpy

from maa.context import ContextEventSink
from maa.define import MaaGlobalOptionEnum
from maa.library import Library
from maa.pipeline import JColorMatch, JDirectHit, JRecognitionType
from maa.resource import Resource
from maa.tasker import Tasker
from test_pipeline_execution import MemoryController


class CaptureSink(ContextEventSink):
    def __init__(self):
        super().__init__()
        self.events = []

    def on_raw_notification(self, context, message, details):
        self.events.append((message, details))
        if message == "Node.Recognition.Starting" and details.get("name") == "Probe":
            context.override_pipeline({"Probe": {"recognition": "DirectHit", "roi": [9, 9, 1, 1]}})


class GlobalCaptureTest(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        root = Path(self.directory.name)
        (root / "pipeline").mkdir()
        (root / "pipeline" / "test.json").write_text(json.dumps({"Probe": {"recognition": "DirectHit", "roi": [1, 2, 3, 4], "action": "DoNothing"}}))
        self.resource = Resource()
        self.assertTrue(self.resource.post_bundle(root).wait().succeeded)
        self.controller = MemoryController()
        self.assertTrue(self.controller.post_connection().wait().succeeded)
        self.first, self.second = Tasker(), Tasker()
        self.assertTrue(self.first.bind(self.resource, self.controller))
        self.assertTrue(self.second.bind(self.resource, self.controller))
        Tasker.set_debug_mode(False)
        Tasker.set_save_draw(False)
        self.assertTrue(Tasker.set_reco_image_raw(False))
        self.assertTrue(Tasker.set_reco_image_draws(False))
        self.assertTrue(Tasker.set_capture_config(False))
        self.assertTrue(Tasker.set_reco_image_cache_limit(4096))
        self.assertTrue(Tasker.set_reco_image_cache_bytes(64 * 1024 * 1024))
        self.image = numpy.zeros((24, 32, 3), dtype=numpy.uint8)

    def tearDown(self):
        Tasker.set_reco_image_raw(False)
        Tasker.set_reco_image_draws(False)
        Tasker.set_capture_config(False)
        Tasker.set_reco_image_cache_limit(4096)
        Tasker.set_reco_image_cache_bytes(ctypes.c_size_t(-1).value)
        del self.first, self.second, self.controller, self.resource
        self.directory.cleanup()

    def recognize(self, tasker, draw=False):
        kind = JRecognitionType.ColorMatch if draw else JRecognitionType.DirectHit
        param = JColorMatch(lower=[[0, 0, 0]], upper=[[255, 255, 255]]) if draw else JDirectHit()
        result = tasker.post_recognition(kind, param, self.image).wait().get()
        self.assertIsNotNone(result)
        return result.nodes[0].recognition

    def test_global_raw_and_draw_policies_apply_to_existing_and_new_taskers(self):
        for raw, draws in [(True, False), (False, True), (True, True), (False, False)]:
            with self.subTest(raw=raw, draws=draws):
                self.assertTrue(Tasker.set_reco_image_raw(raw))
                self.assertTrue(Tasker.set_reco_image_draws(draws))
                for tasker in (self.first, self.second):
                    detail = self.recognize(tasker, draw=True)
                    self.assertEqual(detail.raw_image.size > 0, raw)
                    self.assertEqual(bool(detail.draw_images), draws)
        self.assertTrue(Tasker.set_reco_image_raw(True))
        third = Tasker()
        self.assertTrue(third.bind(self.resource, self.controller))
        numpy.testing.assert_array_equal(self.recognize(third).raw_image, self.image)

    def test_count_byte_budgets_and_native_diagnostics(self):
        Tasker.set_reco_image_raw(True)
        Tasker.set_reco_image_draws(False)
        Tasker.set_reco_image_cache_limit(1)
        sink = CaptureSink()
        self.first.add_context_sink(sink)
        first = self.recognize(self.first)
        other = self.recognize(self.second)
        second = self.recognize(self.first)
        self.assertEqual(self.second.get_recognition_image_status(other.reco_id), "available")
        self.assertEqual(self.first.get_recognition_detail(first.reco_id).raw_image.size, 0)
        self.assertEqual(self.first.get_recognition_image_status(first.reco_id), "evicted")
        self.assertEqual(self.first.get_recognition_image_status(second.reco_id), "available")
        self.assertGreater(self.first.get_recognition_detail(second.reco_id).raw_image.size, 0)
        Tasker.set_reco_image_cache_bytes(1)
        third = self.recognize(self.first)
        self.assertEqual(third.raw_image.size, 0)
        self.assertEqual(self.first.get_recognition_image_status(third.reco_id), "budget_exceeded")
        other = self.recognize(self.second)
        self.assertEqual(self.second.get_recognition_image_status(other.reco_id), "budget_exceeded")
        self.assertIsNone(self.first.get_recognition_image_status(0))
        terminal = [details for message, details in sink.events if message == "Node.Recognition.Succeeded"][-1]
        self.assertEqual(terminal["reco_details"]["image_status"], "budget_exceeded")

    def test_snapshot_is_actual_input_not_later_context_query(self):
        Tasker.set_capture_config(True)
        sink = CaptureSink()
        self.first.add_context_sink(sink)
        self.assertTrue(self.first.post_task("Probe").wait().succeeded)
        recognition = [details for message, details in sink.events if message.startswith("Node.Recognition.")]
        self.assertEqual(len(recognition), 2)
        self.assertEqual(recognition[0]["pipeline"], recognition[1]["pipeline"])
        self.assertEqual(recognition[0]["pipeline"]["recognition"]["param"]["roi"], [1, 2, 3, 4])
        actions = [details for message, details in sink.events if message.startswith("Node.Action.")]
        self.assertEqual(actions[0]["pipeline"]["recognition"]["param"]["roi"], [9, 9, 1, 1])
        self.assertEqual(actions[0]["pipeline"], actions[1]["pipeline"])

    def test_config_capture_is_opt_in(self):
        sink = CaptureSink()
        self.second.add_context_sink(sink)
        self.recognize(self.second)
        self.assertFalse(any("pipeline" in details for _, details in sink.events))
        self.first.add_context_sink(sink)
        Tasker.set_capture_config(True)
        sink.events.clear()
        self.recognize(self.first)
        self.recognize(self.second)
        recognition = [details for message, details in sink.events if message.startswith("Node.Recognition.")]
        self.assertEqual(len(recognition), 4)
        self.assertTrue(all("pipeline" in details for details in recognition))
        with self.assertRaises(ValueError):
            Tasker.set_reco_image_cache_bytes(2**64)
        with self.assertRaises(ValueError):
            Tasker.set_reco_image_cache_bytes(-1)

    def test_global_options_reject_wrong_native_value_sizes(self):
        value = ctypes.c_uint16(1)
        for key in (
            MaaGlobalOptionEnum.RecoImageRaw,
            MaaGlobalOptionEnum.RecoImageDraws,
            MaaGlobalOptionEnum.RecoImageCacheBytes,
            MaaGlobalOptionEnum.CaptureConfig,
        ):
            with self.subTest(option=key):
                self.assertFalse(Library.framework().MaaGlobalSetOption(key, ctypes.byref(value), ctypes.sizeof(value)))
                self.assertFalse(Library.framework().MaaGlobalSetOption(key, None, ctypes.sizeof(value)))


if __name__ == "__main__":
    unittest.main()
