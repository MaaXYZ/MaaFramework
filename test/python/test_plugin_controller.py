"""Exercise a dynamically loaded controller provider through the public Python API."""

import gc
import json
import os
from pathlib import Path
import subprocess
import sys
import unittest

import numpy

from maa.controller import PluginController
from maa.resource import Resource
from maa.tasker import Tasker


class PluginControllerTest(unittest.TestCase):
    def test_load_create_and_release(self):
        # Explicit loading and shutdown operate on process-global plugin state.
        result = subprocess.run([sys.executable, __file__], capture_output=True, timeout=30)
        self.assertEqual(result.returncode, 0, (result.stdout + result.stderr).decode(errors="replace"))

    def check_provider(self):
        plugin_dir = Path(os.environ["MAAFW_BINARY_PATH"]).parent / "test" / "plugins"
        candidates = [p for p in plugin_dir.glob("ControllerFixture.*") if p.suffix in {".so", ".dylib", ".dll"}]
        self.assertEqual(len(candidates), 1, f"Expected the installed ControllerFixture in {plugin_dir}")
        fixture = candidates[0]
        self.assertTrue(Tasker.load_plugin(fixture))
        self.assertTrue(Tasker.load_plugin(fixture))
        self.assertEqual(PluginController.list_names().count("PixelDevice"), 1)
        with self.assertRaises(RuntimeError):
            PluginController("MissingDevice")

        config = {"device": "设备 A"}
        first = PluginController("PixelDevice", config)
        second = PluginController("PixelDevice")
        for device in (first, second):
            self.assertTrue(device.set_screenshot_use_raw_size(True))
            self.assertTrue(device.post_connection().wait().succeeded)
            self.assertTrue(device.post_screencap().wait().succeeded)
        self.assertEqual(json.loads(first.uuid), config)
        self.assertEqual(json.loads(second.uuid), {})
        self.assertEqual(second.info["live_instances"], 2)
        self.assertFalse(Tasker.shutdown_plugins())

        self.assertTrue(first.post_click(0, 0).wait().failed)
        self.assertTrue(first.post_click(1, 0).wait().succeeded)
        pixels = numpy.array([[[1, 2, 3], [4, 5, 6]]], dtype=numpy.uint8)
        numpy.testing.assert_array_equal(second.post_screencap().get(wait=True), pixels)
        pixels[0, 0, 0] = 2
        numpy.testing.assert_array_equal(first.post_screencap().get(wait=True), pixels)

        resource = Resource()
        self.assertTrue(resource.override_pipeline({"Entry": {"pre_delay": 0, "post_delay": 0}}))
        tasker = Tasker()
        self.assertTrue(tasker.bind(resource, second))
        self.assertTrue(tasker.post_task("Entry").wait().succeeded)
        info = second.info
        self.assertGreater(info["controller_events"], 0)
        self.assertGreater(info["context_events"], 0)
        self.assertEqual(info["misrouted_events"], 0)

        del first
        gc.collect()
        self.assertEqual(second.info["live_instances"], 1)
        del tasker, resource, device, second
        gc.collect()
        self.assertTrue(Tasker.shutdown_plugins())
        self.assertNotIn("PixelDevice", PluginController.list_names())
        with self.assertRaises(RuntimeError):
            PluginController("PixelDevice")


if __name__ == "__main__":
    unittest.main(defaultTest="PluginControllerTest.check_provider")
