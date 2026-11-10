"""Device-free tests of the native Pipeline parser, dumper and resource overrides.

Run directly with ``python test_pipeline_contract.py <binding_dir> <install_dir>``.
The module can also be loaded by unittest after configuring the binding/binary paths.
"""

import json
import os
from dataclasses import replace
from pathlib import Path
import sys
import tempfile
import unittest

if __name__ == "__main__":
    if len(sys.argv) < 3:
        raise SystemExit("Usage: python test_pipeline_contract.py <binding_dir> <install_dir>")
    sys.path.insert(0, str(Path(sys.argv[1]).resolve()))
    os.environ["MAAFW_BINARY_PATH"] = str(Path(sys.argv[2]).resolve() / "bin")
    del sys.argv[1:3]

from maa import pipeline as p
from maa.resource import Resource


def parsed_default_recognition(param):
    # A default "self" ROI becomes the whole image when parsed as a node.
    if hasattr(param, "roi") and param.roi is True:
        return replace(param, roi=[0, 0, 0, 0])
    return param


# Each case supplies non-default values and asserts their normalized native dump.
RECOGNITION_CASES = {
    "DirectHit": (
        p.JDirectHit,
        {"roi": [-30, 20, 40, -50], "roi_offset": [1, 2, 3, 4]},
    ),
    "TemplateMatch": (
        p.JTemplateMatch,
        {
            "template": ["模板.png", "second.png"],
            "threshold": [0.65, 0.85],
            "roi": "[Anchor]Location",
            "roi_offset": [1, 2, 3, 4],
            "order_by": "Score",
            "index": -1,
            "method": 3,
            "green_mask": True,
        },
    ),
    "FeatureMatch": (
        p.JFeatureMatch,
        {
            "template": ["feature.png"],
            "detector": "AKAZE",
            "count": 12,
            "ratio": 0.75,
            "roi": [10, 20, 30, 40],
            "roi_offset": [-1, 2, -3, 4],
            "order_by": "Area",
            "index": -2,
            "green_mask": True,
        },
    ),
    "ColorMatch": (
        p.JColorMatch,
        {
            "lower": [[0, 1, 2], [10, 20, 30]],
            "upper": [[50, 60, 70], [200, 210, 220]],
            "roi": [10, 20, 30, 40],
            "roi_offset": [1, 2, 3, 4],
            "method": 40,
            "count": 7,
            "order_by": "Area",
            "index": -1,
            "connected": True,
        },
    ),
    "OCR": (
        p.JOCR,
        {
            "expected": ["测试.*", "second"],
            "replace": [["0", "O"], ["１", "1"]],
            "threshold": 0.55,
            "only_rec": True,
            "model": "custom_ocr",
            "color_filter": "Filter",
            "roi": "[Anchor]Text",
            "roi_offset": [1, 2, 3, 4],
            "order_by": "Expected",
            "index": -1,
        },
    ),
    "NeuralNetworkClassify": (
        p.JNeuralNetworkClassify,
        {
            "model": "classify.onnx",
            "expected": [2, "猫"],
            "labels": ["dog", "mouse", "猫"],
            "roi": [10, 20, 30, 40],
            "roi_offset": [1, 2, 3, 4],
            "order_by": "Expected",
            "index": -1,
        },
    ),
    "NeuralNetworkDetect": (
        p.JNeuralNetworkDetect,
        {
            "model": "detect.onnx",
            "expected": [0, "猫"],
            "labels": ["dog", "猫"],
            "threshold": [0.45, 0.75],
            "roi": [10, 20, 30, 40],
            "roi_offset": [1, 2, 3, 4],
            "order_by": "Score",
            "index": -1,
        },
    ),
    "And": (
        p.JAnd,
        {
            "all_of": ["Reference", {"recognition": "DirectHit", "sub_name": "inline"}],
            "box_index": 1,
        },
    ),
    "Or": (
        p.JOr,
        {"any_of": ["Reference", {"recognition": "DirectHit", "sub_name": "inline"}]},
    ),
    "Custom": (
        p.JCustomRecognition,
        {
            "custom_recognition": "Recognizer",
            "custom_recognition_param": {"文本": [None, True, 42, {"nested": "value"}]},
            "index": -1,
            "roi": [10, 20, 30, 40],
            "roi_offset": [1, 2, 3, 4],
        },
    ),
}

ACTION_CASES = {
    "DoNothing": (p.JDoNothing, {}),
    "Click": (
        p.JClick,
        {
            "target": "[Anchor]Button",
            "target_offset": [1, 2, 3, 4],
            "contact": 2,
            "pressure": 90,
        },
    ),
    "LongPress": (
        p.JLongPress,
        {
            "target": [10, 20, 30, 40],
            "target_offset": [1, 2, 3, 4],
            "duration": 345,
            "contact": 2,
            "pressure": 90,
        },
    ),
    "Swipe": (
        p.JSwipe,
        {
            "begin": [10, 20, 30, 40],
            "begin_offset": [1, 2, 3, 4],
            "end": [[50, 60, 70, 80], "[Anchor]End"],
            "end_offset": [[-1, -2, -3, -4], [1, 2, 3, 4]],
            "end_hold": [12, 34],
            "duration": [123, 456],
            "only_hover": True,
            "contact": 2,
            "pressure": 90,
        },
    ),
    "MultiSwipe": (
        p.JMultiSwipe,
        {
            "swipes": [
                {"begin": [10, 20], "end": [30, 40], "starting": 150, "contact": 1},
                {"begin": True, "end": "Reference", "contact": 2},
            ]
        },
    ),
    "TouchDown": (
        p.JTouch,
        {
            "contact": 2,
            "target": "Reference",
            "target_offset": [1, 2, 3, 4],
            "pressure": 90,
            "auto_up": True,
        },
    ),
    "TouchMove": (
        p.JTouch,
        {
            "contact": 3,
            "target": [10, 20, 30, 40],
            "target_offset": [1, 2, 3, 4],
            "pressure": 80,
            "auto_up": True,
        },
    ),
    "TouchUp": (p.JTouchUp, {"contact": 3}),
    "ClickKey": (p.JClickKey, {"key": [17, 65]}),
    "LongPressKey": (p.JLongPressKey, {"key": [17, 66], "duration": 345}),
    "KeyDown": (p.JKey, {"key": 65, "auto_up": True}),
    "KeyUp": (p.JKey, {"key": 66}),
    "InputText": (p.JInputText, {"input_text": '中文\nquoted "text" 🐈'}),
    "StartApp": (p.JStartApp, {"package": "com.example.test/.MainActivity"}),
    "StopApp": (p.JStopApp, {"package": "com.example.test"}),
    "StopTask": (p.JStopTask, {}),
    "Scroll": (
        p.JScroll,
        {
            "target": "[Anchor]List",
            "target_offset": [1, 2, 3, 4],
            "dx": -120,
            "dy": 240,
        },
    ),
    "Command": (
        p.JCommand,
        {"exec": "never_executed", "args": ["a b", "中文", ""], "detach": True},
    ),
    "Shell": (p.JShell, {"cmd": "echo 'never executed'", "shell_timeout": 3456}),
    "Screencap": (
        p.JScreencap,
        {"filename": "截图/output", "format": "jpeg", "quality": 73},
    ),
    "Custom": (
        p.JCustomAction,
        {
            "custom_action": "Action",
            "custom_action_param": [None, True, {"文本": 42}],
            "target": "Reference",
            "target_offset": [1, 2, 3, 4],
        },
    ),
}

INVALID_NODES = {
    "unknown_recognition": {"recognition": "NotARecognition"},
    "unknown_action": {"action": "NotAnAction"},
    "recognition_type": {"recognition": 42},
    "action_type": {"action": []},
    "enabled_type": {"enabled": "true"},
    "inverse_type": {"inverse": 1},
    "timeout_type": {"timeout": "200"},
    "repeat_type": {"repeat": "2"},
    "next_type": {"next": 42},
    "next_empty_name": {"next": [""]},
    "next_attribute_without_name": {"next": ["[JumpBack]"]},
    "next_unclosed_attribute": {"next": ["[Anchor]Good", "[JumpBackBad"]},
    "next_object_type": {"next": [{"name": 42}]},
    "anchor_type": {"anchor": 42},
    "anchor_array_type": {"anchor": ["valid", 42]},
    "anchor_object_type": {"anchor": {"name": 42}},
    "target_false": {"action": "Click", "target": False},
    "target_empty_rect": {"action": "Click", "target": []},
    "target_three_coordinates": {"action": "Click", "target": [1, 2, 3]},
    "target_non_numeric": {"action": "Click", "target": [1, "2"]},
    "target_offset_type": {"action": "Click", "target_offset": "1,2"},
    "roi_false": {"recognition": "DirectHit", "roi": False},
    "roi_three_coordinates": {"recognition": "DirectHit", "roi": [1, 2, 3]},
    "template_type": {"recognition": "TemplateMatch", "template": 42},
    "template_threshold_type": {
        "recognition": "TemplateMatch",
        "threshold": [0.5, "bad"],
    },
    "template_order": {"recognition": "TemplateMatch", "order_by": "Area"},
    "feature_detector": {"recognition": "FeatureMatch", "detector": "not-a-detector"},
    "ocr_expected_type": {"recognition": "OCR", "expected": [42]},
    "ocr_replace_pair": {"recognition": "OCR", "replace": [["a", "b", "c"]]},
    "ocr_replace_value": {"recognition": "OCR", "replace": [["a", 42]]},
    "detect_threshold_count": {
        "recognition": "NeuralNetworkDetect",
        "expected": [0, 1, 2],
        "threshold": [0.3, 0.5],
    },
    "color_range_count": {
        "recognition": "ColorMatch",
        "lower": [[0], [1]],
        "upper": [[255]],
    },
    "color_channel_count": {
        "recognition": "ColorMatch",
        "lower": [[0, 0]],
        "upper": [[255, 255, 255]],
    },
    "color_too_many_channels": {
        "recognition": "ColorMatch",
        "lower": [[0] * 5],
        "upper": [[255] * 5],
    },
    "and_empty": {"recognition": "And", "all_of": []},
    "and_missing": {"recognition": "And"},
    "and_negative_box_index": {
        "recognition": "And",
        "all_of": ["Reference"],
        "box_index": -1,
    },
    "and_large_box_index": {
        "recognition": "And",
        "all_of": ["Reference"],
        "box_index": 1,
    },
    "or_empty": {"recognition": "Or", "any_of": []},
    "or_non_array": {"recognition": "Or", "any_of": "Reference"},
    "swipe_end_type": {"action": "Swipe", "end": [False]},
    "swipe_duration_type": {"action": "Swipe", "duration": [100, "bad"]},
    "multi_swipe_type": {"action": "MultiSwipe", "swipes": {}},
    "click_key_type": {"action": "ClickKey", "key": [65, "bad"]},
    "key_down_array": {"action": "KeyDown", "key": [65]},
    "input_text_type": {"action": "InputText", "input_text": 42},
    "command_args_type": {"action": "Command", "args": ["valid", 42]},
    "shell_timeout_type": {"action": "Shell", "shell_timeout": "1000"},
    "screencap_format": {"action": "Screencap", "format": "gif"},
    "pre_wait_freezes_type": {"pre_wait_freezes": True},
    "post_wait_freezes_type": {"post_wait_freezes": []},
    "repeat_wait_freezes_type": {"repeat_wait_freezes": "100"},
    "wait_freezes_target": {"pre_wait_freezes": {"target": False}},
    "wait_freezes_time": {"post_wait_freezes": {"time": "100"}},
    "deprecated_interrupt": {"interrupt": ["Reference"]},
}


class PipelineContractTest(unittest.TestCase):
    def setUp(self):
        self.resource = Resource()

    def assert_roundtrip(self, node):
        self.assertTrue(self.resource.override_pipeline({"Node": node}))
        dumped = self.resource.get_node_data("Node")
        self.assertIsNotNone(dumped)
        restored = Resource()
        self.assertTrue(restored.override_pipeline({"Node": dumped}))
        self.assertEqual(restored.get_node_data("Node"), dumped)
        return dumped

    def test_all_recognition_types_have_cases(self):
        self.assertEqual(set(RECOGNITION_CASES), set(p.JRecognitionType))

    def test_all_action_types_have_cases(self):
        self.assertEqual(set(ACTION_CASES), set(p.JActionType))

    def test_custom_recognition_rejects_invalid_index_types(self):
        for index in ("-1", True, [1], {"index": 1}):
            for v2 in (False, True):
                with self.subTest(index=index, v2=v2):
                    param = {"custom_recognition": "Recognizer", "index": index}
                    node = (
                        {"recognition": {"type": "Custom", "param": param}}
                        if v2
                        else {"recognition": "Custom", **param}
                    )
                    self.assertFalse(self.resource.override_pipeline({"Invalid": node}))
                    self.assertIsNone(self.resource.get_node_data("Invalid"))
        self.assertTrue(self.resource.override_pipeline({"Valid": {}}))

    def test_custom_recognition_rejects_framework_sort_order(self):
        for field in ("order", "order_by"):
            for v2 in (False, True):
                with self.subTest(field=field, v2=v2):
                    param = {"custom_recognition": "Recognizer", field: "Horizontal"}
                    node = (
                        {"recognition": {"type": "Custom", "param": param}}
                        if v2
                        else {"recognition": "Custom", **param}
                    )
                    self.assertFalse(self.resource.override_pipeline({"Invalid": node}))
                    self.assertIsNone(self.resource.get_node_data("Invalid"))
        self.assertTrue(self.resource.override_pipeline({"Valid": {}}))

    def test_common_fields_roundtrip(self):
        wait = {
            "time": 123,
            "target": "[Anchor]Frozen",
            "target_offset": [1, 2, 3, 4],
            "threshold": 0.8,
            "method": 3,
            "rate_limit": 25,
            "timeout": 456,
        }
        node = {
            "next": [
                {"name": "Next", "jump_back": True, "anchor": False},
                {"name": "Back", "jump_back": True, "anchor": True},
            ],
            "on_error": [{"name": "Recovery", "jump_back": False, "anchor": True}],
            "anchor": {"Location": "OtherNode"},
            "rate_limit": 12,
            "timeout": 345,
            "inverse": True,
            "enabled": False,
            "pre_delay": 23,
            "post_delay": 45,
            "pre_wait_freezes": wait,
            "post_wait_freezes": wait,
            "repeat": 3,
            "repeat_delay": 67,
            "repeat_wait_freezes": wait,
            "max_hit": 8,
            "focus": {"Node.Action.Succeeded": "message"},
            "attach": {"中文": [None, True, 42, {"nested": "value"}]},
        }
        dumped = self.assert_roundtrip(node)
        for key, value in node.items():
            with self.subTest(field=key):
                self.assertEqual(dumped[key], value)

    def test_partial_override_preserves_other_parameters(self):
        original = {
            "recognition": {
                "type": "OCR",
                "param": {
                    "expected": ["original"],
                    "model": "custom",
                    "only_rec": True,
                },
            },
            "action": {"type": "Click", "param": {"target": [10, 20], "pressure": 90}},
            "timeout": 1234,
        }
        self.assertTrue(self.resource.override_pipeline({"Node": original}))
        self.assertTrue(
            self.resource.override_pipeline(
                {
                    "Node": {
                        "recognition": {"param": {"expected": ["changed"]}},
                        "action": {"param": {"contact": 2}},
                    }
                }
            )
        )
        node = self.resource.get_node_object("Node")
        self.assertEqual(node.recognition.param.expected, ["changed"])
        self.assertEqual(node.recognition.param.model, "custom")
        self.assertTrue(node.recognition.param.only_rec)
        self.assertEqual(node.action.param.target, [10, 20, 1, 1])
        self.assertEqual(node.action.param.pressure, 90)
        self.assertEqual(node.action.param.contact, 2)
        self.assertEqual(node.timeout, 1234)

    def test_changing_algorithm_uses_new_algorithm_defaults(self):
        self.assertTrue(
            self.resource.override_pipeline(
                {
                    "Node": {
                        "recognition": "OCR",
                        "expected": ["old"],
                        "roi": [10, 20],
                        "action": "LongPress",
                        "target": [30, 40],
                        "duration": 999,
                    }
                }
            )
        )
        self.assertTrue(self.resource.override_pipeline({"Node": {"recognition": "ColorMatch", "action": "Click"}}))
        node = self.resource.get_node_object("Node")
        self.assertIsInstance(node.recognition.param, p.JColorMatch)
        self.assertEqual(
            node.recognition.param,
            parsed_default_recognition(self.resource.get_default_recognition_param(p.JRecognitionType.ColorMatch)),
        )
        self.assertEqual(
            node.action.param,
            self.resource.get_default_action_param(p.JActionType.Click),
        )

    def test_empty_override_preserves_node(self):
        self.assertTrue(
            self.resource.override_pipeline(
                {
                    "Node": {
                        "recognition": "OCR",
                        "expected": ["keep"],
                        "action": "Click",
                        "target": [1, 2],
                    }
                }
            )
        )
        original = self.resource.get_node_data("Node")
        self.assertTrue(self.resource.override_pipeline({"Node": {}}))
        self.assertEqual(self.resource.get_node_data("Node"), original)

    def test_attach_merges_top_level_keys(self):
        self.assertTrue(self.resource.override_pipeline({"Node": {"attach": {"keep": 1, "replace": {"a": 2}}}}))
        self.assertTrue(self.resource.override_pipeline({"Node": {"attach": {"replace": {"b": 3}, "new": None}}}))
        self.assertEqual(
            self.resource.get_node_data("Node")["attach"],
            {"keep": 1, "replace": {"b": 3}, "new": None},
        )

    def test_explicit_empty_collections_clear_inherited_values(self):
        self.assertTrue(
            self.resource.override_pipeline(
                {
                    "Node": {
                        "recognition": "OCR",
                        "expected": ["old"],
                        "replace": [["a", "b"]],
                        "next": ["Next"],
                        "on_error": ["Error"],
                        "anchor": ["Anchor"],
                    }
                }
            )
        )
        self.assertTrue(
            self.resource.override_pipeline(
                {
                    "Node": {
                        "expected": [],
                        "replace": [],
                        "next": [],
                        "on_error": [],
                        "anchor": {},
                    }
                }
            )
        )
        node = self.resource.get_node_object("Node")
        self.assertEqual(node.recognition.param.expected, [])
        self.assertEqual(node.recognition.param.replace, [])
        self.assertEqual(node.next, [])
        self.assertEqual(node.on_error, [])
        self.assertEqual(node.anchor, {})

    def test_resource_overrides_are_isolated(self):
        other = Resource()
        self.assertTrue(self.resource.override_pipeline({"Node": {"attach": {"value": 1}}}))
        self.assertIsNone(other.get_node_data("Node"))
        self.assertTrue(other.override_pipeline({"Node": {"attach": {"value": 2}}}))
        self.assertEqual(self.resource.get_node_data("Node")["attach"], {"value": 1})

    def test_returned_data_is_detached(self):
        self.assertTrue(self.resource.override_pipeline({"Node": {"attach": {"nested": [1, 2]}}}))
        data = self.resource.get_node_data("Node")
        data["attach"]["nested"].append(3)
        self.assertEqual(self.resource.get_node_data("Node")["attach"], {"nested": [1, 2]})

    def test_ignored_nodes_do_not_appear(self):
        self.assertTrue(self.resource.override_pipeline({"$comment": "ignored even when not an object", "Node": {}}))
        self.assertEqual(self.resource.node_list, ["Node"])
        self.assertIsNone(self.resource.get_node_data("$comment"))

    def test_invalid_node_name_and_value(self):
        for pipeline in ({"": {}}, {"Node": None}, {"Node": []}, {"Node": 42}):
            with self.subTest(pipeline=pipeline):
                self.assertFalse(self.resource.override_pipeline(pipeline))
                self.assertEqual(self.resource.node_list, [])

    def test_node_attribute_string_normalization(self):
        dumped = self.assert_roundtrip(
            {
                "next": ["[JumpBack][Anchor]Return", "Plain"],
                "on_error": "[Anchor]Recover",
                "anchor": "Current",
            }
        )
        self.assertEqual(
            dumped["next"],
            [
                {"name": "Return", "jump_back": True, "anchor": True},
                {"name": "Plain", "jump_back": False, "anchor": False},
            ],
        )
        self.assertEqual(
            dumped["on_error"],
            [{"name": "Recover", "jump_back": False, "anchor": True}],
        )
        self.assertEqual(dumped["anchor"], {"Current": "Node"})

    def test_enabled_alias(self):
        self.assertTrue(self.resource.override_pipeline({"Node": {"enable": False}}))
        self.assertFalse(self.resource.get_node_object("Node").enabled)
        self.assertTrue(self.resource.override_pipeline({"Node": {"enabled": True}}))
        self.assertTrue(self.resource.get_node_object("Node").enabled)

    def test_screencap_quality_clamps(self):
        for quality, expected in ((-1, 0), (0, 0), (100, 100), (101, 100)):
            with self.subTest(quality=quality):
                self.assertTrue(self.resource.override_pipeline({"Node": {"action": "Screencap", "quality": quality}}))
                self.assertEqual(self.resource.get_node_object("Node").action.param.quality, expected)

    def test_detect_scalar_threshold_broadcast(self):
        self.assertTrue(
            self.resource.override_pipeline(
                {
                    "Node": {
                        "recognition": "NeuralNetworkDetect",
                        "expected": [1, 2, "label"],
                        "threshold": 0.75,
                    }
                }
            )
        )
        self.assertEqual(
            self.resource.get_node_object("Node").recognition.param.threshold,
            [0.75, 0.75, 0.75],
        )

    def test_scalar_and_short_form_normalization(self):
        cases = [
            (
                {
                    "recognition": "TemplateMatch",
                    "template": "single.png",
                    "threshold": 0.75,
                },
                "recognition",
                {"template": ["single.png"], "threshold": [0.75]},
            ),
            (
                {
                    "recognition": "ColorMatch",
                    "lower": [0, 1, 2],
                    "upper": [250, 251, 252],
                },
                "recognition",
                {"lower": [[0, 1, 2]], "upper": [[250, 251, 252]]},
            ),
            (
                {"recognition": "OCR", "expected": "文字", "replace": ["0", "O"]},
                "recognition",
                {"expected": ["文字"], "replace": [["0", "O"]]},
            ),
            (
                {
                    "recognition": "NeuralNetworkClassify",
                    "expected": "猫",
                    "labels": "猫",
                },
                "recognition",
                {"expected": ["猫"], "labels": ["猫"]},
            ),
            ({"action": "ClickKey", "key": 65}, "action", {"key": [65]}),
            (
                {
                    "action": "Swipe",
                    "begin": [1, 2],
                    "end": [3, 4],
                    "end_offset": [5, 6],
                    "duration": 123,
                    "end_hold": 45,
                },
                "action",
                {
                    "begin": [1, 2, 1, 1],
                    "end": [[3, 4, 1, 1]],
                    "end_offset": [[5, 6, 1, 1]],
                    "duration": [123],
                    "end_hold": [45],
                },
            ),
        ]
        for node, section, expected in cases:
            with self.subTest(node=node):
                self.resource = Resource()
                dumped = self.assert_roundtrip(node)
                for key, value in expected.items():
                    self.assertEqual(dumped[section]["param"][key], value)

    def test_wait_freezes_scalar_and_partial_override(self):
        for key in ("pre_wait_freezes", "post_wait_freezes", "repeat_wait_freezes"):
            with self.subTest(field=key):
                self.assertTrue(
                    self.resource.override_pipeline(
                        {"Node": {key: {"time": 100, "threshold": 0.8, "target": [10, 20]}}}
                    )
                )
                self.assertTrue(self.resource.override_pipeline({"Node": {key: 250}}))
                value = self.resource.get_node_data("Node")[key]
                self.assertEqual(value["time"], 250)
                self.assertEqual(value["threshold"], 0.8)
                self.assertEqual(value["target"], [10, 20, 1, 1])


class PipelineFileTest(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)
        self.resource = Resource()

    def write_json(self, relative, data):
        path = self.root / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(json.dumps(data, ensure_ascii=False), encoding="utf-8")
        return path

    def test_pipeline_file_and_bundle_overrides(self):
        first = self.write_json(
            "base/pipeline/main.json",
            {"Node": {"recognition": "OCR", "expected": ["base"], "timeout": 1234}},
        )
        second = self.write_json(
            "override/pipeline/main.json",
            {"Node": {"expected": ["updated"]}, "New": {}},
        )
        self.assertTrue(self.resource.post_bundle(first.parent.parent).wait().succeeded)
        self.assertTrue(self.resource.post_bundle(second.parent.parent).wait().succeeded)
        self.assertEqual(
            self.resource.get_node_object("Node").recognition.param.expected,
            ["updated"],
        )
        self.assertEqual(self.resource.get_node_object("Node").timeout, 1234)
        self.assertEqual(set(self.resource.node_list), {"Node", "New"})

    def test_post_pipeline_file_and_jsonc(self):
        path = self.root / "配置.jsonc"
        path.write_text(
            '// comment\n{"中文节点": {"action": "InputText", "input_text": "你好",},}',
            encoding="utf-8",
        )
        self.assertTrue(self.resource.post_pipeline(path).wait().succeeded)
        self.assertEqual(self.resource.get_node_object("中文节点").action.param.input_text, "你好")

    def test_hidden_files_and_directories_are_ignored(self):
        self.write_json("pipeline/visible.json", {"Visible": {}})
        self.write_json("pipeline/.hidden.json", {"Hidden": {}})
        self.write_json("pipeline/.hidden/sub.json", {"HiddenSub": {}})
        (self.root / "pipeline" / "ignored.txt").write_text("invalid JSON", encoding="utf-8")
        self.assertTrue(self.resource.post_pipeline(self.root / "pipeline").wait().succeeded)
        self.assertEqual(self.resource.node_list, ["Visible"])

    def test_duplicate_nodes_in_one_directory_fail(self):
        self.write_json("pipeline/first.json", {"Duplicate": {}})
        self.write_json("pipeline/second.json", {"Duplicate": {}})
        self.assertTrue(self.resource.post_pipeline(self.root / "pipeline").wait().failed)

    def test_invalid_json_file_fails(self):
        path = self.root / "invalid.json"
        path.write_text('{"Node":', encoding="utf-8")
        self.assertTrue(self.resource.post_pipeline(path).wait().failed)

    def test_non_object_pipeline_file_fails(self):
        path = self.write_json("invalid.json", [1, 2, 3])
        self.assertTrue(self.resource.post_pipeline(path).wait().failed)

    def test_missing_pipeline_file_fails(self):
        self.assertTrue(self.resource.post_pipeline(self.root / "missing.json").wait().failed)

    def test_default_pipeline_and_algorithm_parameters(self):
        self.write_json(
            "default_pipeline.json",
            {
                "Default": {"timeout": 3456, "pre_delay": 12, "post_delay": 34},
                "OCR": {
                    "recognition": "OCR",
                    "expected": ["default text"],
                    "model": "custom",
                    "only_rec": True,
                },
                "Click": {"action": "Click", "target": [10, 20], "pressure": 90},
            },
        )
        self.write_json(
            "pipeline/main.json",
            {
                "Inherited": {"recognition": "OCR", "action": "Click"},
                "Explicit": {
                    "recognition": "OCR",
                    "expected": ["explicit"],
                    "action": "Click",
                    "pressure": 50,
                    "timeout": 789,
                },
            },
        )
        self.assertTrue(self.resource.post_bundle(self.root).wait().succeeded)
        inherited = self.resource.get_node_object("Inherited")
        self.assertEqual(inherited.timeout, 3456)
        self.assertEqual(inherited.pre_delay, 12)
        self.assertEqual(inherited.post_delay, 34)
        self.assertEqual(
            inherited.recognition.param,
            self.resource.get_default_recognition_param(p.JRecognitionType.OCR),
        )
        self.assertEqual(
            inherited.action.param,
            self.resource.get_default_action_param(p.JActionType.Click),
        )
        self.assertEqual(inherited.recognition.param.expected, ["default text"])
        self.assertEqual(inherited.action.param.target, [10, 20, 1, 1])
        explicit = self.resource.get_node_object("Explicit")
        self.assertEqual(explicit.recognition.param.expected, ["explicit"])
        self.assertEqual(explicit.recognition.param.model, "custom")
        self.assertEqual(explicit.action.param.pressure, 50)
        self.assertEqual(explicit.timeout, 789)


def recognition_case_test(type_name, param_class, params):
    def test(self):
        for v2 in (False, True):
            with self.subTest(v2=v2):
                self.resource = Resource()
                node = (
                    {"recognition": {"type": type_name, "param": params}}
                    if v2
                    else {"recognition": type_name, **params}
                )
                dumped = self.assert_roundtrip(node)
                self.assertEqual(dumped["recognition"]["type"], type_name)
                obj = self.resource.get_node_object("Node")
                self.assertIsInstance(obj.recognition.param, param_class)
                for key, value in params.items():
                    if key not in ("all_of", "any_of"):
                        self.assertEqual(dumped["recognition"]["param"][key], value)
                if type_name in ("And", "Or"):
                    key = "all_of" if type_name == "And" else "any_of"
                    items = dumped["recognition"]["param"][key]
                    self.assertEqual(items[0], "Reference")
                    self.assertEqual(items[1]["sub_name"], "inline")
                    self.assertEqual(items[1]["recognition"]["type"], "DirectHit")

    return test


def action_case_test(type_name, param_class, params):
    def test(self):
        for v2 in (False, True):
            with self.subTest(v2=v2):
                self.resource = Resource()
                node = {"action": {"type": type_name, "param": params}} if v2 else {"action": type_name, **params}
                dumped = self.assert_roundtrip(node)
                self.assertEqual(dumped["action"]["type"], type_name)
                obj = self.resource.get_node_object("Node")
                self.assertIsInstance(obj.action.param, param_class)
                for key, value in params.items():
                    if key != "swipes":
                        self.assertEqual(dumped["action"]["param"][key], value)
                if type_name == "MultiSwipe":
                    swipes = dumped["action"]["param"]["swipes"]
                    self.assertEqual(len(swipes), 2)
                    self.assertEqual(swipes[0]["starting"], 150)
                    self.assertEqual(swipes[0]["begin"], [10, 20, 1, 1])
                    self.assertEqual(swipes[0]["end"], [[30, 40, 1, 1]])
                    self.assertEqual(swipes[1]["end"], ["Reference"])
                    self.assertEqual([s["contact"] for s in swipes], [1, 2])

    return test


def invalid_case_test(node):
    def test(self):
        self.assertFalse(self.resource.override_pipeline({"Invalid": node}))
        self.assertIsNone(self.resource.get_node_data("Invalid"))
        self.assertTrue(self.resource.override_pipeline({"Valid": {}}))
        self.assertIsNotNone(self.resource.get_node_object("Valid"))

    return test


def default_param_test(section, type_name, param_class):
    def test(self):
        if section == "recognition":
            actual = self.resource.get_default_recognition_param(p.JRecognitionType(type_name))
        else:
            actual = self.resource.get_default_action_param(p.JActionType(type_name))
        self.assertIsInstance(actual, param_class)
        if section == "recognition":
            actual = parsed_default_recognition(actual)
        if type_name not in ("And", "Or"):
            self.assertTrue(self.resource.override_pipeline({"Node": {section: type_name}}))
            node = self.resource.get_node_object("Node")
            self.assertEqual(getattr(node, section).param, actual)

    return test


for _name, (_param_class, _params) in RECOGNITION_CASES.items():
    setattr(
        PipelineContractTest,
        f"test_recognition_roundtrip_{_name}",
        recognition_case_test(_name, _param_class, _params),
    )
    setattr(
        PipelineContractTest,
        f"test_default_recognition_{_name}",
        default_param_test("recognition", _name, _param_class),
    )

for _name, (_param_class, _params) in ACTION_CASES.items():
    setattr(
        PipelineContractTest,
        f"test_action_roundtrip_{_name}",
        action_case_test(_name, _param_class, _params),
    )
    setattr(
        PipelineContractTest,
        f"test_default_action_{_name}",
        default_param_test("action", _name, _param_class),
    )

for _name, _node in INVALID_NODES.items():
    setattr(PipelineContractTest, f"test_reject_{_name}", invalid_case_test(_node))


def run_pipeline_contract_tests():
    suite = unittest.defaultTestLoader.loadTestsFromModule(sys.modules[__name__])
    result = unittest.TextTestRunner(verbosity=2).run(suite)
    if not result.wasSuccessful():
        raise SystemExit(1)


if __name__ == "__main__":
    unittest.main()
