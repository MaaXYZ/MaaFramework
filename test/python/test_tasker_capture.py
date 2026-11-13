"""Per-instance evidence policy and execution-time configuration regression tests."""
import json
from pathlib import Path
import tempfile
import unittest

import numpy

from maa.context import ContextEventSink
from maa.pipeline import JColorMatch, JDirectHit, JRecognitionType
from maa.resource import Resource
from maa.tasker import Tasker


class CaptureSink(ContextEventSink):
    def __init__(self):
        super().__init__()
        self.events = []

    def on_raw_notification(self, context, message, details):
        self.events.append((message, details))
        if message == "Node.Recognition.Starting" and details.get("name") == "Probe":
            context.override_pipeline({"Probe": {"recognition": "DirectHit", "roi": [9, 9, 1, 1]}})


class TaskerCaptureTest(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        root = Path(self.directory.name)
        (root / "pipeline").mkdir()
        (root / "pipeline" / "test.json").write_text(json.dumps({"Probe": {"recognition": "DirectHit", "roi": [1, 2, 3, 4], "action": "DoNothing"}}))
        self.resource = Resource()
        self.assertTrue(self.resource.post_bundle(root).wait().succeeded)
        self.first, self.second = Tasker(), Tasker()
        self.assertTrue(self.first.bind(self.resource, None))
        self.assertTrue(self.second.bind(self.resource, None))
        Tasker.set_debug_mode(False)
        Tasker.set_save_draw(False)
        self.image = numpy.zeros((24, 32, 3), dtype=numpy.uint8)

    def tearDown(self):
        del self.first, self.second, self.resource
        self.directory.cleanup()

    def recognize(self, tasker, draw=False):
        kind = JRecognitionType.ColorMatch if draw else JRecognitionType.DirectHit
        param = JColorMatch(lower=[[0, 0, 0]], upper=[[255, 255, 255]]) if draw else JDirectHit()
        result = tasker.post_recognition(kind, param, self.image).wait().get()
        self.assertIsNotNone(result)
        return result.nodes[0].recognition

    def test_independent_raw_and_draw_policies(self):
        self.assertTrue(self.first.set_reco_image_raw(True))
        self.assertTrue(self.first.set_reco_image_draws(True))
        self.assertTrue(self.second.set_reco_image_raw(False))
        self.assertTrue(self.second.set_reco_image_draws(False))
        first = self.recognize(self.first, draw=True)
        second = self.recognize(self.second, draw=True)
        numpy.testing.assert_array_equal(first.raw_image, self.image)
        self.assertGreater(len(first.draw_images), 0)
        self.assertIsNone(second.raw_image)
        self.assertEqual(second.draw_images, [])

    def test_count_byte_budgets_and_native_diagnostics(self):
        self.first.set_reco_image_raw(True)
        self.first.set_reco_image_draws(False)
        self.first.set_reco_image_cache_count(1)
        sink = CaptureSink()
        self.first.add_context_sink(sink)
        first = self.recognize(self.first)
        second = self.recognize(self.first)
        self.assertIsNone(self.first.get_recognition_detail(first.reco_id).raw_image)
        self.assertEqual(self.first.get_recognition_image_status(first.reco_id), "evicted")
        self.assertEqual(self.first.get_recognition_image_status(second.reco_id), "available")
        self.assertIsNotNone(self.first.get_recognition_detail(second.reco_id).raw_image)
        self.first.set_reco_image_cache_bytes(1)
        third = self.recognize(self.first)
        self.assertIsNone(third.raw_image)
        self.assertEqual(self.first.get_recognition_image_status(third.reco_id), "budget_exceeded")
        self.assertIsNone(self.first.get_recognition_image_status(0))
        terminal = [details for message, details in sink.events if message == "Node.Recognition.Succeeded"][-1]
        self.assertEqual(terminal["reco_details"]["image_status"], "budget_exceeded")

    def test_snapshot_is_actual_input_not_later_context_query(self):
        self.first.set_capture_config(True)
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
        with self.assertRaises(ValueError):
            self.second.set_reco_image_cache_count(-1)
        with self.assertRaises(ValueError):
            self.second.set_reco_image_cache_bytes(2**64)


if __name__ == "__main__":
    unittest.main()
