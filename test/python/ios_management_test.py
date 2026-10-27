"""Python consumer tests against an XDB1 peer; no device is accessed."""
from pathlib import Path
import sys
import unittest
from types import SimpleNamespace
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / '3rdparty/XDeviceBridge/tests/support'))
from fake_server import FakeServer
from maa.controller import IOSController
from maa.library import Library
from maa.toolkit import Toolkit


class IOSClientTest(unittest.TestCase):
    def test_discovery_and_controller_use_server_settings(self):
        with FakeServer() as server:
            devices = Toolkit.find_ios_devices(timeout_ms=0, config=server.config)
            self.assertEqual([d.udid for d in devices], ['mock-iphone'])
            status = Toolkit.ios_discover(options={'timeout_ms': 0}, config=server.config)
            self.assertTrue(status['success'])
            controller = IOSController('mock-iphone', config=server.config, wifi_address='auto')
            self.assertTrue(controller.post_connection().wait().succeeded)
            self.assertTrue(controller.post_screencap().wait().succeeded)
            del controller
            self.assertNotIn('pair', server.methods())
            self.assertNotIn('ddi.prepare', server.methods())

    def test_invalid_inputs(self):
        with self.assertRaises(ValueError):
            Toolkit.find_ios_devices(timeout_ms=-1)
        with self.assertRaises(ValueError):
            IOSController('')
        with self.assertRaises(TypeError):
            Toolkit.ios_discover(options=[])

    def test_already_cancelled_discovery(self):
        with FakeServer() as server:
            status = Toolkit.ios_discover(config=server.config, cancelled=lambda: True)
            self.assertFalse(status['success'])
            self.assertTrue(status['cancelled'])
            self.assertNotIn('devices', server.methods())

    def test_older_toolkit_reports_missing_ios_lazily(self):
        previous = Toolkit._ios_api_properties_initialized
        Toolkit._ios_api_properties_initialized = False
        try:
            with patch.object(Library, 'toolkit', return_value=SimpleNamespace()):
                with self.assertRaisesRegex(RuntimeError, 'not available'):
                    Toolkit.find_ios_devices(timeout_ms=0)
        finally:
            Toolkit._ios_api_properties_initialized = previous


if __name__ == '__main__':
    unittest.main()
