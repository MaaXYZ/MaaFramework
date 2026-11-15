"""Exercise process-global plugin management in an isolated interpreter."""
import subprocess
import sys
import textwrap
import unittest


class PluginManagementTest(unittest.TestCase):
    def test_shutdown_requires_destroyed_instances(self):
        code = textwrap.dedent("""
            import gc
            from maa.tasker import Tasker
            from maa.resource import Resource
            assert isinstance(Tasker.get_plugin_list(), list)
            resource = Resource()
            assert not Tasker.shutdown_plugins()
            assert Tasker.flush_plugins()
            del resource
            gc.collect()
            assert Tasker.shutdown_plugins()
            assert Tasker.shutdown_plugins()
            assert not Tasker.flush_plugins()
        """)
        result = subprocess.run([sys.executable, "-c", code], capture_output=True, text=True, timeout=30)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
