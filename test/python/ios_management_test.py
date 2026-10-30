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


class DiscoveryServer(FakeServer):
    state = 'paired'

    def handle_call(self, name, params):
        result, data = super().handle_call(name, params)
        if name == 'devices':
            result['devices'][0]['state'] = self.state
        return result, data


class IOSClientTest(unittest.TestCase):
    def test_discovery_and_controller_use_server_settings(self):
        with DiscoveryServer() as server:
            devices = Toolkit.find_ios_devices(timeout_ms=0, config=server.config)
            self.assertEqual([d.udid for d in devices], ['mock-iphone'])
            self.assertEqual(devices[0].state, 'paired')
            for state in ('discovered', 'connected'):
                server.state = state
                self.assertEqual(Toolkit.find_ios_devices(timeout_ms=0, config=server.config)[0].state, state)
            self.assertEqual(devices[0].connections[0]['address'], '127.0.0.2')
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
            Toolkit.find_ios_devices(config=[])

    def test_already_cancelled_discovery(self):
        with FakeServer() as server:
            with self.assertRaisesRegex(RuntimeError, 'cancelled'):
                Toolkit.find_ios_devices(config=server.config, cancelled=lambda: True)
            self.assertNotIn('devices', server.methods())

    def test_cancellation_exception_is_rethrown_after_cleanup(self):
        def cancelled():
            raise ValueError('cancel callback failed')

        with FakeServer() as server:
            with self.assertRaisesRegex(ValueError, 'cancel callback failed'):
                Toolkit.find_ios_devices(config=server.config, cancelled=cancelled)
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
