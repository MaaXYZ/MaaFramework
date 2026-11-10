"""Deterministic Pipeline traversal tests with an in-memory controller."""

from pathlib import Path
import tempfile
import unittest

import numpy as np

from maa.controller import CustomController
from maa.custom_action import CustomAction
from maa.tasker import Tasker
from maa.resource import Resource


class MemoryController(CustomController):
    def __init__(self):
        super().__init__()
        self.screenshot_count = 0
        self.unexpected_inputs = []

    def connect(self):
        return True

    def request_uuid(self):
        return "pipeline-execution-test"

    def get_features(self):
        # Avoid coordinate-scaling initialization screenshots.
        return 1 << 2

    def screencap(self):
        self.screenshot_count += 1
        return np.zeros((64, 64, 3), dtype=np.uint8)

    def unexpected_input(self, *args):
        self.unexpected_inputs.append(args)
        return False

    click = unexpected_input
    swipe = unexpected_input
    touch_down = unexpected_input
    touch_move = unexpected_input
    touch_up = unexpected_input
    click_key = unexpected_input
    input_text = unexpected_input
    key_down = unexpected_input
    key_up = unexpected_input
    start_app = unexpected_input
    stop_app = unexpected_input


class RecordingAction(CustomAction):
    def __init__(self):
        super().__init__()
        self.calls = []
        self.hit_counts = []
        self.results = {}
        self.hook = None
        self.errors = []

    def run(self, context, argv):
        self.calls.append(argv.node_name)
        self.hit_counts.append(context.get_hit_count(argv.node_name))
        if self.hook:
            try:
                self.hook(context, argv)
            except Exception as error:
                # ctypes callbacks cannot propagate assertions to the test runner.
                self.errors.append(error)
                return False
        results = self.results.get(argv.node_name, [])
        return results.pop(0) if results else True


class PipelineExecutionTest(unittest.TestCase):
    def setUp(self):
        self.bundle = tempfile.TemporaryDirectory()
        self.addCleanup(self.bundle.cleanup)
        pipeline = Path(self.bundle.name) / "pipeline"
        pipeline.mkdir()
        (pipeline / "empty.json").write_text("{}", encoding="utf-8")
        self.resource = Resource()
        self.assertTrue(self.resource.post_bundle(self.bundle.name).wait().succeeded)
        self.controller = MemoryController()
        self.assertTrue(self.controller.post_connection().wait().succeeded)
        self.action = RecordingAction()
        self.assertTrue(self.resource.register_custom_action("Record", self.action))
        self.tasker = Tasker()
        self.assertTrue(self.tasker.bind(self.resource, self.controller))
        self.addCleanup(self.stop_tasker)

    def tearDown(self):
        self.action.hook = None

    def stop_tasker(self):
        if self.tasker.running:
            self.tasker.post_stop().wait()

    @staticmethod
    def node(**overrides):
        return {
            "recognition": "DirectHit",
            "action": "Custom",
            "custom_action": "Record",
            "target": [0, 0, 1, 1],
            "pre_delay": 0,
            "post_delay": 0,
            "rate_limit": 0,
            "timeout": 0,
            **overrides,
        }

    def run_pipeline(self, nodes, expected, succeeded=True):
        job = self.tasker.post_task("Entry", nodes).wait()
        self.assertEqual(job.succeeded, succeeded)
        self.assertEqual(job.failed, not succeeded)
        self.assertEqual(self.action.errors, [])
        self.assertEqual(self.action.calls, expected)
        self.assertEqual(self.controller.unexpected_inputs, [])
        detail = job.get()
        self.assertIsNotNone(detail)
        return detail

    def test_next_uses_first_matching_node(self):
        detail = self.run_pipeline(
            {
                "Entry": self.node(next=["First", "Second"]),
                "First": self.node(),
                "Second": self.node(),
            },
            ["Entry", "First"],
        )
        self.assertEqual([node.name for node in detail.nodes], ["Entry", "First"])
        self.assertEqual(self.controller.screenshot_count, 0)

    def test_disabled_node_is_skipped(self):
        self.run_pipeline(
            {
                "Entry": self.node(next=["Disabled", "Enabled"]),
                "Disabled": self.node(enabled=False),
                "Enabled": self.node(),
            },
            ["Entry", "Enabled"],
        )

    def test_disabled_entry_fails_without_action(self):
        self.run_pipeline({"Entry": self.node(enabled=False)}, [], succeeded=False)

    def test_inverse_rejects_a_matching_recognition(self):
        self.run_pipeline(
            {
                "Entry": self.node(next=["Inverted", "Fallback"]),
                "Inverted": self.node(inverse=True),
                "Fallback": self.node(),
            },
            ["Entry", "Fallback"],
        )

    def test_inverse_accepts_a_missing_recognition(self):
        self.run_pipeline(
            {
                "Entry": self.node(next=["Inverted", "Fallback"]),
                "Inverted": self.node(
                    recognition="ColorMatch",
                    lower=[255, 255, 255],
                    upper=[255, 255, 255],
                    inverse=True,
                ),
                "Fallback": self.node(),
            },
            ["Entry", "Inverted"],
        )
        self.assertGreater(self.controller.screenshot_count, 0)

    def test_max_hit_limits_a_loop(self):
        self.run_pipeline(
            {
                "Entry": self.node(next=["Loop"]),
                "Loop": self.node(max_hit=2, next=["Loop", "Finished"]),
                "Finished": self.node(),
            },
            ["Entry", "Loop", "Loop", "Finished"],
        )
        self.assertEqual(self.action.hit_counts, [1, 1, 2, 1])

    def test_max_hit_zero_skips_node(self):
        self.run_pipeline(
            {
                "Entry": self.node(next=["Never", "Finished"]),
                "Never": self.node(max_hit=0),
                "Finished": self.node(),
            },
            ["Entry", "Finished"],
        )

    def test_hit_counts_reset_for_each_task(self):
        nodes = {
            "Entry": self.node(next=["Loop"]),
            "Loop": self.node(max_hit=1, next=["Loop", "Finished"]),
            "Finished": self.node(),
        }
        for _ in range(2):
            self.action.calls.clear()
            self.action.hit_counts.clear()
            self.run_pipeline(nodes, ["Entry", "Loop", "Finished"])
            self.assertEqual(self.action.hit_counts, [1, 1, 1])

    def test_failed_action_uses_its_on_error(self):
        self.action.results["Failing"] = [False]
        detail = self.run_pipeline(
            {
                "Entry": self.node(next=["Failing"]),
                "Failing": self.node(next=["Wrong"], on_error=["Recovery"]),
                "Wrong": self.node(),
                "Recovery": self.node(),
            },
            ["Entry", "Failing", "Recovery"],
        )
        self.assertEqual([node.completed for node in detail.nodes], [True, False, True])

    def test_recognition_timeout_uses_previous_node_on_error(self):
        self.run_pipeline(
            {
                "Entry": self.node(next=["Never"], on_error=["Recovery"]),
                "Never": self.node(inverse=True, on_error=["Wrong"]),
                "Recovery": self.node(),
                "Wrong": self.node(),
            },
            ["Entry", "Recovery"],
        )

    def test_missing_recovery_match_fails_and_terminates(self):
        self.action.results["Entry"] = [False]
        self.run_pipeline(
            {
                "Entry": self.node(on_error=["Never"]),
                "Never": self.node(inverse=True, on_error=["Entry"]),
            },
            ["Entry"],
            succeeded=False,
        )

    def test_jump_back_resumes_original_next_list(self):
        detail = self.run_pipeline(
            {
                "Entry": self.node(next=["[JumpBack]Sub", "Finished"]),
                "Sub": self.node(max_hit=1, next=["Leaf"]),
                "Leaf": self.node(),
                "Finished": self.node(),
            },
            ["Entry", "Sub", "Leaf", "Finished"],
        )
        self.assertEqual([node.name for node in detail.nodes], ["Entry", "Sub", "Leaf", "Finished"])

    def test_anchor_resolves_target_node(self):
        self.run_pipeline(
            {
                "Entry": self.node(anchor={"Route": "Target"}, next=["[Anchor]Route"]),
                "Target": self.node(),
            },
            ["Entry", "Target"],
        )

    def test_anchor_update_redirects_traversal(self):
        self.run_pipeline(
            {
                "Entry": self.node(anchor={"Route": "Old"}, next=["Update"]),
                "Update": self.node(anchor={"Route": "New"}, next=["[Anchor]Route"]),
                "Old": self.node(),
                "New": self.node(),
            },
            ["Entry", "Update", "New"],
        )

    def test_cleared_anchor_is_skipped(self):
        self.run_pipeline(
            {
                "Entry": self.node(anchor={"Route": "Old"}, next=["Clear"]),
                "Clear": self.node(anchor={"Route": ""}, next=["[Anchor]Route", "Finished"]),
                "Old": self.node(),
                "Finished": self.node(),
            },
            ["Entry", "Clear", "Finished"],
        )

    def test_repeat_runs_actions_without_repeating_recognition(self):
        detail = self.run_pipeline(
            {"Entry": self.node(repeat=3, next=["Finished"]), "Finished": self.node()},
            ["Entry", "Entry", "Entry", "Finished"],
        )
        self.assertEqual(self.action.hit_counts, [1, 1, 1, 1])
        self.assertEqual([node.name for node in detail.nodes], ["Entry", "Finished"])

    def test_repeat_continues_after_failure_and_uses_last_result(self):
        self.action.results["Entry"] = [False, True, True]
        self.run_pipeline(
            {
                "Entry": self.node(repeat=3, next=["Finished"], on_error=["Wrong"]),
                "Finished": self.node(),
                "Wrong": self.node(),
            },
            ["Entry", "Entry", "Entry", "Finished"],
        )

    def test_repeat_last_failure_uses_on_error(self):
        self.action.results["Entry"] = [True, True, False]
        self.run_pipeline(
            {
                "Entry": self.node(repeat=3, next=["Wrong"], on_error=["Recovery"]),
                "Wrong": self.node(),
                "Recovery": self.node(),
            },
            ["Entry", "Entry", "Entry", "Recovery"],
        )

    def test_context_clone_override_is_isolated(self):
        def check_clone(context, argv):
            clone = context.clone()
            self.assertTrue(clone.override_pipeline({"Other": {"attach": {"value": "clone"}}}))
            self.assertEqual(clone.get_node_data("Other")["attach"], {"value": "clone"})
            self.assertEqual(context.get_node_data("Other")["attach"], {"value": "resource"})
            self.assertEqual(self.resource.get_node_data("Other")["attach"], {"value": "resource"})

        self.assertTrue(self.resource.override_pipeline({"Other": {"attach": {"value": "resource"}}}))
        self.action.hook = check_clone
        self.run_pipeline({"Entry": self.node()}, ["Entry"])


if __name__ == "__main__":
    unittest.main()
