"""
Pipeline 解析、override_pipeline 和 get_node_data/get_node_object 测试

测试范围：
1. Resource 级别: 加载资源后的 get_node_data / get_node_object
2. Context 级别: override_pipeline / override_next 后的 get_node_data / get_node_object
3. Pipeline 各字段解析: recognition, action, next, on_error, delays, wait_freezes, etc.
4. v1/v2 格式兼容性
5. And/Or 复合识别
6. Node 属性: jump_back, anchor
"""

import os
from pathlib import Path
import sys
import json
import io

# Fix encoding issues on Windows
if sys.stdout.encoding != "utf-8":
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding="utf-8", errors="replace")
if sys.stderr.encoding != "utf-8":
    sys.stderr = io.TextIOWrapper(sys.stderr.buffer, encoding="utf-8", errors="replace")

if len(sys.argv) < 3:
    print("Usage: python pipeline_test.py <binding_dir> <install_dir>")
    sys.exit(1)

binding_dir = Path(sys.argv[1]).resolve()
install_dir = Path(sys.argv[2]).resolve()

os.environ["MAAFW_BINARY_PATH"] = str(f"{install_dir}/bin")
print(f"binding_dir: {binding_dir}")
print(f"install_dir: {install_dir}")

if str(binding_dir) not in sys.path:
    sys.path.insert(0, str(binding_dir))

from maa.library import Library
from maa.resource import Resource
from maa.controller import DbgController
from maa.tasker import Tasker
from maa.toolkit import Toolkit
from maa.custom_action import CustomAction
from maa.custom_recognition import CustomRecognition
from maa.define import MaaDbgControllerTypeEnum, LoggingLevelEnum
from maa.context import Context
from maa.pipeline import (
    JPipelineData,
    JRecognitionType,
    JActionType,
    JDirectHit,
    JTemplateMatch,
    JFeatureMatch,
    JColorMatch,
    JOCR,
    JNeuralNetworkClassify,
    JNeuralNetworkDetect,
    JAnd,
    JOr,
    JCustomRecognition,
    JClick,
    JLongPress,
    JSwipe,
    JMultiSwipe,
    JInputText,
    JStartApp,
    JStopApp,
    JScroll,
    JCommand,
    JShell,
    JCustomAction,
    JNodeAttr,
)


def assert_eq(actual, expected, msg=""):
    if actual != expected:
        raise AssertionError(f"{msg}: expected {expected!r}, got {actual!r}")


def assert_true(condition, msg=""):
    if not condition:
        raise AssertionError(f"{msg}: expected True")


def assert_not_none(value, msg=""):
    if value is None:
        raise AssertionError(f"{msg}: expected not None")


# ============================================================================
# Resource 级别测试
# ============================================================================


def test_resource_get_node_data(resource: Resource):
    """测试 Resource.get_node_data 返回原始 JSON dict"""
    print("\n=== test_resource_get_node_data ===")

    # 测试存在的节点
    node_data = resource.get_node_data("TestBasic")
    assert_not_none(node_data, "TestBasic node_data")
    assert_true(isinstance(node_data, dict), "node_data should be dict")

    # 检查基本字段存在
    assert_true("recognition" in node_data, "recognition field")
    assert_true("action" in node_data, "action field")

    # 测试不存在的节点
    non_exist = resource.get_node_data("NonExistentNode12345")
    assert_eq(non_exist, None, "non-existent node should return None")

    print("  PASS: resource.get_node_data")


def test_resource_get_node_object(resource: Resource):
    """测试 Resource.get_node_object 返回解析后的 JPipelineData 对象"""
    print("\n=== test_resource_get_node_object ===")

    node_obj = resource.get_node_object("TestBasic")
    assert_not_none(node_obj, "TestBasic node_obj")
    assert_true(isinstance(node_obj, JPipelineData), "should be JPipelineData")

    # 验证 recognition
    assert_eq(node_obj.recognition.type, JRecognitionType.DirectHit, "reco type")

    # 验证 action
    assert_eq(node_obj.action.type, JActionType.DoNothing, "action type")

    print("  PASS: resource.get_node_object")


def test_resource_node_list(resource: Resource):
    """测试 Resource.node_list 获取所有节点名列表"""
    print("\n=== test_resource_node_list ===")

    node_list = resource.node_list
    assert_true(isinstance(node_list, list), "node_list should be list")
    assert_true(len(node_list) > 0, "node_list should not be empty")
    assert_true("TestBasic" in node_list, "TestBasic should be in node_list")

    print(f"  Found {len(node_list)} nodes: {node_list[:5]}...")
    print("  PASS: resource.node_list")


# ============================================================================
# Context 级别测试
# ============================================================================


class PipelineTestRecognition(CustomRecognition):
    """用于测试 Context 级别 pipeline 操作的自定义识别器"""

    test_results = {}

    def analyze(
        self, context: Context, argv: CustomRecognition.AnalyzeArg
    ) -> CustomRecognition.AnalyzeResult:
        print(
            f"\n=== PipelineTestRecognition.analyze: {argv.custom_recognition_name} ==="
        )

        try:
            self._run_context_tests(context)
            PipelineTestRecognition.test_results["context_tests"] = True
        except Exception as e:
            print(f"  FAIL: {e}")
            PipelineTestRecognition.test_results["context_tests"] = False
            raise

        return CustomRecognition.AnalyzeResult(box=(0, 0, 100, 100), detail="test")

    def _run_context_tests(self, context: Context):
        # 1. 测试 context.get_node_data
        self._test_context_get_node_data(context)

        # 2. 测试 context.get_node_object
        self._test_context_get_node_object(context)

        # 3. 测试 context.override_pipeline
        self._test_context_override_pipeline(context)

        # 4. 测试 context.override_next
        self._test_context_override_next(context)

        # 5. 测试 And/Or 识别的 override 继承
        self._test_and_or_override_inheritance(context)

        # 5.5 测试 And/Or 节点名称引用
        self._test_and_or_node_reference(context)

        # 6. 测试各种识别类型的解析
        self._test_recognition_types(context)

        # 7. 测试各种动作类型的解析
        self._test_action_types(context)

        # 8. 测试 Node 属性
        self._test_node_attributes(context)

        # 9. 测试 anchor 对象格式
        self._test_anchor_object_format(context)

        # 10. 测试 v2 格式
        self._test_v2_format(context)

        # 11. 比较 Context 和 Resource 级别
        self._test_context_vs_resource(context)

    def _test_context_get_node_data(self, context: Context):
        print("  Testing context.get_node_data...")

        # 获取原始资源中的节点
        node_data = context.get_node_data("TestBasic")
        assert_not_none(node_data, "context get_node_data")
        assert_true(isinstance(node_data, dict), "should be dict")
        print("    PASS: context.get_node_data")

    def _test_context_get_node_object(self, context: Context):
        print("  Testing context.get_node_object...")

        node_obj = context.get_node_object("TestBasic")
        assert_not_none(node_obj, "context get_node_object")
        assert_true(isinstance(node_obj, JPipelineData), "should be JPipelineData")
        print("    PASS: context.get_node_object")

    def _test_context_override_pipeline(self, context: Context):
        print("  Testing context.override_pipeline...")

        new_ctx = context.clone()

        # 先创建被引用的节点（框架会检查 next 列表中的节点是否存在）
        new_ctx.override_pipeline({"NodeA": {}, "NodeB": {}})

        # 创建新节点
        new_ctx.override_pipeline(
            {
                "OverrideTestNode": {
                    "recognition": "OCR",
                    "expected": ["TestText"],
                    "action": "Click",
                    "target": [100, 200, 50, 50],
                    "next": ["NodeA", "NodeB"],
                    "timeout": 5000,
                    "rate_limit": 500,
                    "pre_delay": 100,
                    "post_delay": 300,
                    "max_hit": 3,
                    "enabled": True,
                    "inverse": False,
                    "anchor": ["my_anchor"],
                    "focus": {"key": "value"},
                    "attach": {"custom_data": 123},
                }
            }
        )

        node_obj = new_ctx.get_node_object("OverrideTestNode")
        assert_not_none(node_obj, "override node should exist")

        # 验证 recognition
        assert_eq(node_obj.recognition.type, JRecognitionType.OCR, "reco type")
        assert_true(isinstance(node_obj.recognition.param, JOCR), "reco param type")
        assert_eq(node_obj.recognition.param.expected, ["TestText"], "expected")

        # 验证 action
        assert_eq(node_obj.action.type, JActionType.Click, "action type")
        assert_true(isinstance(node_obj.action.param, JClick), "action param type")

        # 验证其他字段
        assert_eq(node_obj.timeout, 5000, "timeout")
        assert_eq(node_obj.rate_limit, 500, "rate_limit")
        assert_eq(node_obj.pre_delay, 100, "pre_delay")
        assert_eq(node_obj.post_delay, 300, "post_delay")
        assert_eq(node_obj.max_hit, 3, "max_hit")
        assert_eq(node_obj.enabled, True, "enabled")
        assert_eq(node_obj.inverse, False, "inverse")
        assert_eq(node_obj.anchor, {"my_anchor": "OverrideTestNode"}, "anchor")  # Array format converted to dict with node name
        assert_eq(node_obj.attach.get("custom_data"), 123, "attach")

        # 验证 next 列表解析
        assert_eq(len(node_obj.next), 2, "next length")
        assert_eq(node_obj.next[0].name, "NodeA", "next[0].name")
        assert_eq(node_obj.next[1].name, "NodeB", "next[1].name")

        print("    PASS: context.override_pipeline")

    def _test_context_override_next(self, context: Context):
        print("  Testing context.override_next...")

        new_ctx = context.clone()

        # 先创建被引用的节点（框架会检查 next 列表中的节点是否存在）
        # 对于 [Anchor] 节点，它是锚点引用而不是真正的节点，所以不需要创建
        new_ctx.override_pipeline(
            {
                "OverrideNextTestNode": {},
                "NextNode1": {},
                "NextNode2": {},
                "MyAnchor": {"anchor": ["MyAnchor"]},  # 创建一个带 anchor 的节点
            }
        )

        # 使用 override_next 修改 next 列表
        result = new_ctx.override_next(
            "OverrideNextTestNode",
            ["NextNode1", "[JumpBack]NextNode2", "[Anchor]MyAnchor"],
        )
        assert_true(result, "override_next should succeed")

        node_obj = new_ctx.get_node_object("OverrideNextTestNode")
        assert_eq(len(node_obj.next), 3, "next length after override")
        assert_eq(node_obj.next[0].name, "NextNode1", "next[0].name")
        assert_eq(node_obj.next[0].jump_back, False, "next[0].jump_back")
        assert_eq(node_obj.next[1].name, "NextNode2", "next[1].name")
        assert_eq(node_obj.next[1].jump_back, True, "next[1].jump_back")
        assert_eq(node_obj.next[2].name, "MyAnchor", "next[2].name")
        assert_eq(node_obj.next[2].anchor, True, "next[2].anchor")

        print("    PASS: context.override_next")

    def _test_and_or_override_inheritance(self, context: Context):
        print("  Testing And/Or override inheritance...")

        new_ctx = context.clone()

        # 创建 And 识别节点
        new_ctx.override_pipeline(
            {
                "AndTestNode": {
                    "recognition": {
                        "type": "And",
                        "param": {
                            "all_of": [
                                {"recognition": {"type": "DirectHit"}},
                                {"recognition": {"type": "DirectHit"}},
                            ],
                            "box_index": 1,
                        },
                    },
                },
            }
        )

        # 只修改 box_index，all_of 应该被继承
        new_ctx.override_pipeline(
            {
                "AndTestNode": {
                    "recognition": {"param": {"box_index": 0}},
                },
            }
        )

        and_node_obj = new_ctx.get_node_object("AndTestNode")
        assert_eq(and_node_obj.recognition.type, JRecognitionType.And, "And type")
        assert_true(isinstance(and_node_obj.recognition.param, JAnd), "And param type")
        assert_eq(
            len(and_node_obj.recognition.param.all_of), 2, "all_of should be inherited"
        )
        assert_eq(
            and_node_obj.recognition.param.box_index, 0, "box_index should be updated"
        )

        # 创建 Or 识别节点
        new_ctx.override_pipeline(
            {
                "OrTestNode": {
                    "recognition": {
                        "type": "Or",
                        "param": {
                            "any_of": [
                                {"recognition": {"type": "DirectHit"}},
                            ],
                        },
                    },
                },
            }
        )

        # 用空对象 override，any_of 应该被继承
        new_ctx.override_pipeline({"OrTestNode": {}})

        or_node_obj = new_ctx.get_node_object("OrTestNode")
        assert_eq(or_node_obj.recognition.type, JRecognitionType.Or, "Or type")
        assert_true(isinstance(or_node_obj.recognition.param, JOr), "Or param type")
        assert_eq(
            len(or_node_obj.recognition.param.any_of), 1, "any_of should be inherited"
        )

        print("    PASS: And/Or override inheritance")

    def _test_and_or_node_reference(self, context: Context):
        print("  Testing And/Or node name reference...")

        new_ctx = context.clone()

        # 创建基础识别节点
        new_ctx.override_pipeline(
            {
                "BaseTemplateNode": {
                    "recognition": "TemplateMatch",
                    "template": ["test.png"],
                    "threshold": 0.8,
                },
                "BaseOCRNode": {
                    "recognition": "OCR",
                    "expected": ["hello"],
                },
            }
        )

        # 创建 And 节点，使用节点名称引用
        new_ctx.override_pipeline(
            {
                "AndWithNodeRef": {
                    "recognition": {
                        "type": "And",
                        "param": {
                            "all_of": [
                                "BaseTemplateNode",  # 节点名称引用
                                "BaseOCRNode",  # 节点名称引用
                                {"recognition": {"type": "DirectHit"}},  # 内联定义
                            ],
                            "box_index": 0,
                        },
                    },
                },
            }
        )

        and_node_obj = new_ctx.get_node_object("AndWithNodeRef")
        assert_eq(and_node_obj.recognition.type, JRecognitionType.And, "And type")
        assert_true(isinstance(and_node_obj.recognition.param, JAnd), "And param type")
        # all_of 应该有 3 个元素：2 个字符串引用 + 1 个内联对象
        assert_eq(len(and_node_obj.recognition.param.all_of), 3, "all_of length")
        # 字符串引用保持为字符串
        assert_eq(
            and_node_obj.recognition.param.all_of[0],
            "BaseTemplateNode",
            "all_of[0] is node name reference",
        )
        assert_eq(
            and_node_obj.recognition.param.all_of[1],
            "BaseOCRNode",
            "all_of[1] is node name reference",
        )
        # 内联定义是对象
        assert_true(
            isinstance(and_node_obj.recognition.param.all_of[2], dict),
            "all_of[2] is inline object",
        )

        # 创建 Or 节点，使用节点名称引用
        new_ctx.override_pipeline(
            {
                "OrWithNodeRef": {
                    "recognition": {
                        "type": "Or",
                        "param": {
                            "any_of": [
                                "BaseTemplateNode",  # 节点名称引用
                                {"recognition": {"type": "DirectHit"}},  # 内联定义
                            ],
                        },
                    },
                },
            }
        )

        or_node_obj = new_ctx.get_node_object("OrWithNodeRef")
        assert_eq(or_node_obj.recognition.type, JRecognitionType.Or, "Or type")
        assert_true(isinstance(or_node_obj.recognition.param, JOr), "Or param type")
        assert_eq(len(or_node_obj.recognition.param.any_of), 2, "any_of length")
        assert_eq(
            or_node_obj.recognition.param.any_of[0],
            "BaseTemplateNode",
            "any_of[0] is node name reference",
        )
        assert_true(
            isinstance(or_node_obj.recognition.param.any_of[1], dict),
            "any_of[1] is inline object",
        )

        print("    PASS: And/Or node name reference")

    def _test_recognition_types(self, context: Context):
        print("  Testing recognition types parsing...")

        new_ctx = context.clone()

        # TemplateMatch
        new_ctx.override_pipeline(
            {
                "RecoTemplateMatch": {
                    "recognition": "TemplateMatch",
                    "template": ["test.png"],
                    "threshold": 0.8,
                    "roi": [10, 20, 100, 200],
                    "order_by": "Score",
                    "index": 1,
                    "method": 3,
                    "green_mask": True,
                }
            }
        )
        obj = new_ctx.get_node_object("RecoTemplateMatch")
        assert_eq(
            obj.recognition.type, JRecognitionType.TemplateMatch, "TemplateMatch type"
        )
        param = obj.recognition.param
        assert_true(isinstance(param, JTemplateMatch), "TemplateMatch param")
        assert_eq(param.template, ["test.png"], "template")
        assert_eq(param.threshold, [0.8], "threshold")
        assert_eq(param.order_by, "Score", "order_by")
        assert_eq(param.index, 1, "index")
        assert_eq(param.method, 3, "method")
        assert_eq(param.green_mask, True, "green_mask")

        # FeatureMatch
        new_ctx.override_pipeline(
            {
                "RecoFeatureMatch": {
                    "recognition": "FeatureMatch",
                    "template": ["feature.png"],
                    "detector": "ORB",
                    "count": 10,
                    "ratio": 0.75,
                }
            }
        )
        obj = new_ctx.get_node_object("RecoFeatureMatch")
        assert_eq(
            obj.recognition.type, JRecognitionType.FeatureMatch, "FeatureMatch type"
        )
        param = obj.recognition.param
        assert_true(isinstance(param, JFeatureMatch), "FeatureMatch param")
        assert_eq(param.detector, "ORB", "detector")
        assert_eq(param.count, 10, "count")
        assert_eq(param.ratio, 0.75, "ratio")

        # ColorMatch
        new_ctx.override_pipeline(
            {
                "RecoColorMatch": {
                    "recognition": "ColorMatch",
                    "lower": [[100, 100, 100]],
                    "upper": [[255, 255, 255]],
                    "count": 50,
                    "method": 40,
                    "connected": True,
                }
            }
        )
        obj = new_ctx.get_node_object("RecoColorMatch")
        assert_eq(obj.recognition.type, JRecognitionType.ColorMatch, "ColorMatch type")
        param = obj.recognition.param
        assert_true(isinstance(param, JColorMatch), "ColorMatch param")
        assert_eq(param.lower, [[100, 100, 100]], "lower")
        assert_eq(param.upper, [[255, 255, 255]], "upper")
        assert_eq(param.count, 50, "count")
        assert_eq(param.method, 40, "method")
        assert_eq(param.connected, True, "connected")

        # OCR
        new_ctx.override_pipeline(
            {
                "RecoOCR": {
                    "recognition": "OCR",
                    "expected": ["Hello", "World"],
                    "threshold": 0.5,
                    "replace": [["0", "O"], ["1", "I"]],
                    "only_rec": True,
                    "model": "custom_model",
                }
            }
        )
        obj = new_ctx.get_node_object("RecoOCR")
        assert_eq(obj.recognition.type, JRecognitionType.OCR, "OCR type")
        param = obj.recognition.param
        assert_true(isinstance(param, JOCR), "OCR param")
        assert_eq(param.expected, ["Hello", "World"], "expected")
        assert_eq(param.threshold, 0.5, "threshold")
        assert_eq(param.replace, [["0", "O"], ["1", "I"]], "replace")
        assert_eq(param.only_rec, True, "only_rec")
        assert_eq(param.model, "custom_model", "model")

        # NeuralNetworkClassify
        new_ctx.override_pipeline(
            {
                "RecoNNClassify": {
                    "recognition": "NeuralNetworkClassify",
                    "model": "classify.onnx",
                    "expected": [0, 2],
                    "labels": ["Cat", "Dog", "Mouse"],
                }
            }
        )
        obj = new_ctx.get_node_object("RecoNNClassify")
        assert_eq(
            obj.recognition.type,
            JRecognitionType.NeuralNetworkClassify,
            "NNClassify type",
        )
        param = obj.recognition.param
        assert_true(isinstance(param, JNeuralNetworkClassify), "NNClassify param")
        assert_eq(param.model, "classify.onnx", "model")
        assert_eq(param.expected, [0, 2], "expected")
        assert_eq(param.labels, ["Cat", "Dog", "Mouse"], "labels")

        # NeuralNetworkDetect
        new_ctx.override_pipeline(
            {
                "RecoNNDetect": {
                    "recognition": "NeuralNetworkDetect",
                    "model": "detect.onnx",
                    "expected": [1],
                    "threshold": [0.5],
                }
            }
        )
        obj = new_ctx.get_node_object("RecoNNDetect")
        assert_eq(
            obj.recognition.type, JRecognitionType.NeuralNetworkDetect, "NNDetect type"
        )
        param = obj.recognition.param
        assert_true(isinstance(param, JNeuralNetworkDetect), "NNDetect param")
        assert_eq(param.model, "detect.onnx", "model")
        assert_eq(param.expected, [1], "expected")
        assert_eq(param.threshold, [0.5], "threshold")

        # Custom
        new_ctx.override_pipeline(
            {
                "RecoCustom": {
                    "recognition": "Custom",
                    "custom_recognition": "MyCustomReco",
                    "custom_recognition_param": {"key": "value"},
                    "roi": [0, 0, 100, 100],
                }
            }
        )
        obj = new_ctx.get_node_object("RecoCustom")
        assert_eq(obj.recognition.type, JRecognitionType.Custom, "Custom type")
        param = obj.recognition.param
        assert_true(isinstance(param, JCustomRecognition), "Custom param")
        assert_eq(param.custom_recognition, "MyCustomReco", "custom_recognition")
        assert_eq(
            param.custom_recognition_param, {"key": "value"}, "custom_recognition_param"
        )

        print("    PASS: recognition types parsing")

    def _test_action_types(self, context: Context):
        print("  Testing action types parsing...")

        new_ctx = context.clone()

        # Click
        new_ctx.override_pipeline(
            {
                "ActClick": {
                    "action": "Click",
                    "target": [100, 200, 50, 50],
                    "target_offset": [10, 10, 0, 0],
                    "contact": 1,
                }
            }
        )
        obj = new_ctx.get_node_object("ActClick")
        assert_eq(obj.action.type, JActionType.Click, "Click type")
        param = obj.action.param
        assert_true(isinstance(param, JClick), "Click param")
        assert_eq(param.contact, 1, "contact")

        # LongPress
        new_ctx.override_pipeline(
            {"ActLongPress": {"action": "LongPress", "duration": 2000}}
        )
        obj = new_ctx.get_node_object("ActLongPress")
        assert_eq(obj.action.type, JActionType.LongPress, "LongPress type")
        param = obj.action.param
        assert_true(isinstance(param, JLongPress), "LongPress param")
        assert_eq(param.duration, 2000, "duration")

        # Swipe
        new_ctx.override_pipeline(
            {
                "ActSwipe": {
                    "action": "Swipe",
                    "begin": [100, 100],
                    "end": [300, 300],
                    "duration": 500,
                }
            }
        )
        obj = new_ctx.get_node_object("ActSwipe")
        assert_eq(obj.action.type, JActionType.Swipe, "Swipe type")
        param = obj.action.param
        assert_true(isinstance(param, JSwipe), "Swipe param")

        # MultiSwipe
        new_ctx.override_pipeline(
            {
                "ActMultiSwipe": {
                    "action": "MultiSwipe",
                    "swipes": [
                        {"begin": [100, 100], "end": [200, 200]},
                        {"starting": 500, "begin": [300, 300], "end": [400, 400]},
                    ],
                }
            }
        )
        obj = new_ctx.get_node_object("ActMultiSwipe")
        assert_eq(obj.action.type, JActionType.MultiSwipe, "MultiSwipe type")
        param = obj.action.param
        assert_true(isinstance(param, JMultiSwipe), "MultiSwipe param")
        assert_eq(len(param.swipes), 2, "swipes count")

        # InputText
        new_ctx.override_pipeline(
            {"ActInputText": {"action": "InputText", "input_text": "Hello World"}}
        )
        obj = new_ctx.get_node_object("ActInputText")
        assert_eq(obj.action.type, JActionType.InputText, "InputText type")
        param = obj.action.param
        assert_true(isinstance(param, JInputText), "InputText param")
        assert_eq(param.input_text, "Hello World", "input_text")

        # StartApp
        new_ctx.override_pipeline(
            {"ActStartApp": {"action": "StartApp", "package": "com.example.app"}}
        )
        obj = new_ctx.get_node_object("ActStartApp")
        assert_eq(obj.action.type, JActionType.StartApp, "StartApp type")
        param = obj.action.param
        assert_true(isinstance(param, JStartApp), "StartApp param")
        assert_eq(param.package, "com.example.app", "package")

        # StopApp
        new_ctx.override_pipeline(
            {"ActStopApp": {"action": "StopApp", "package": "com.example.app"}}
        )
        obj = new_ctx.get_node_object("ActStopApp")
        assert_eq(obj.action.type, JActionType.StopApp, "StopApp type")
        param = obj.action.param
        assert_true(isinstance(param, JStopApp), "StopApp param")
        assert_eq(param.package, "com.example.app", "package")

        # Command
        new_ctx.override_pipeline(
            {
                "ActCommand": {
                    "action": "Command",
                    "exec": "python",
                    "args": ["script.py", "--arg1"],
                    "detach": True,
                }
            }
        )
        obj = new_ctx.get_node_object("ActCommand")
        assert_eq(obj.action.type, JActionType.Command, "Command type")
        param = obj.action.param
        assert_true(isinstance(param, JCommand), "Command param")
        assert_eq(param.exec, "python", "exec")
        assert_eq(param.args, ["script.py", "--arg1"], "args")
        assert_eq(param.detach, True, "detach")

        # Shell
        new_ctx.override_pipeline(
            {"ActShell": {"action": "Shell", "cmd": "ls -la", "shell_timeout": 30000}}
        )
        obj = new_ctx.get_node_object("ActShell")
        assert_eq(obj.action.type, JActionType.Shell, "Shell type")
        param = obj.action.param
        assert_true(isinstance(param, JShell), "Shell param")
        assert_eq(param.cmd, "ls -la", "cmd")
        assert_eq(param.shell_timeout, 30000, "shell_timeout")

        # Custom
        new_ctx.override_pipeline(
            {
                "ActCustom": {
                    "action": "Custom",
                    "custom_action": "MyCustomAction",
                    "custom_action_param": {"data": 123},
                }
            }
        )
        obj = new_ctx.get_node_object("ActCustom")
        assert_eq(obj.action.type, JActionType.Custom, "Custom type")
        param = obj.action.param
        assert_true(isinstance(param, JCustomAction), "Custom param")
        assert_eq(param.custom_action, "MyCustomAction", "custom_action")
        assert_eq(param.custom_action_param, {"data": 123}, "custom_action_param")

        # Scroll
        new_ctx.override_pipeline(
            {
                "ActScroll": {
                    "action": "Scroll",
                    "target": [100, 200, 50, 50],
                    "target_offset": [10, 10, 0, 0],
                    "dx": 0,
                    "dy": -360,
                }
            }
        )
        obj = new_ctx.get_node_object("ActScroll")
        assert_eq(obj.action.type, JActionType.Scroll, "Scroll type")
        param = obj.action.param
        assert_true(isinstance(param, JScroll), "Scroll param")
        assert_eq(param.target, [100, 200, 50, 50], "target")
        assert_eq(param.target_offset, [10, 10, 0, 0], "target_offset")
        assert_eq(param.dx, 0, "dx")
        assert_eq(param.dy, -360, "dy")

        print("    PASS: action types parsing")

    def _test_node_attributes(self, context: Context):
        print("  Testing node attributes...")

        new_ctx = context.clone()

        # 先创建被引用的节点（框架会检查 next 列表中的节点是否存在）
        # 注意：[Anchor] 节点是锚点引用，需要有对应的锚点
        new_ctx.override_pipeline(
            {
                "PlainNode": {},
                "JumpBackNode": {},
                "AnchorRef": {"anchor": ["AnchorRef"]},  # 创建带 anchor 的节点
                "ObjectNode": {},
                "AnchorObjNode": {"anchor": ["AnchorObjNode"]},
                "ErrorHandler": {},
            }
        )

        # 测试 next 列表中的节点属性
        new_ctx.override_pipeline(
            {
                "NodeAttrTest": {
                    "next": [
                        "PlainNode",
                        "[JumpBack]JumpBackNode",
                        "[Anchor]AnchorRef",
                        {"name": "ObjectNode", "jump_back": True},
                        {"name": "AnchorObjNode", "anchor": True},
                    ],
                    "on_error": ["[JumpBack]ErrorHandler"],
                }
            }
        )

        obj = new_ctx.get_node_object("NodeAttrTest")

        # 验证 next
        assert_eq(len(obj.next), 5, "next length")
        assert_eq(obj.next[0].name, "PlainNode", "next[0].name")
        assert_eq(obj.next[0].jump_back, False, "next[0].jump_back")
        assert_eq(obj.next[0].anchor, False, "next[0].anchor")

        assert_eq(obj.next[1].name, "JumpBackNode", "next[1].name")
        assert_eq(obj.next[1].jump_back, True, "next[1].jump_back")

        assert_eq(obj.next[2].name, "AnchorRef", "next[2].name")
        assert_eq(obj.next[2].anchor, True, "next[2].anchor")

        assert_eq(obj.next[3].name, "ObjectNode", "next[3].name")
        assert_eq(obj.next[3].jump_back, True, "next[3].jump_back")

        assert_eq(obj.next[4].name, "AnchorObjNode", "next[4].name")
        assert_eq(obj.next[4].anchor, True, "next[4].anchor")

        # 验证 on_error
        assert_eq(len(obj.on_error), 1, "on_error length")
        assert_eq(obj.on_error[0].name, "ErrorHandler", "on_error[0].name")
        assert_eq(obj.on_error[0].jump_back, True, "on_error[0].jump_back")

        print("    PASS: node attributes")

    def _test_anchor_object_format(self, context: Context):
        print("  Testing anchor object format...")

        new_ctx = context.clone()

        # 测试 anchor 的三种格式
        new_ctx.override_pipeline(
            {
                # 格式 1: 字符串
                "AnchorString": {
                    "anchor": "StringAnchor"
                },
                # 格式 2: 字符串数组
                "AnchorArray": {
                    "anchor": ["ArrayAnchor1", "ArrayAnchor2"]
                },
                # 格式 3: 对象 - 映射到特定节点或清除
                "AnchorObject": {
                    "anchor": {
                        "ObjAnchor1": "TargetNode1",
                        "ObjAnchor2": "",  # 空字符串表示清除锚点
                        "ObjAnchor3": "TargetNode2"
                    }
                },
                # 目标节点
                "TargetNode1": {},
                "TargetNode2": {},
            }
        )

        # 验证格式 1: 字符串 (解析时直接设置为当前节点名)
        obj1 = new_ctx.get_node_object("AnchorString")
        assert_not_none(obj1, "AnchorString should exist")
        assert_eq(obj1.anchor, {"StringAnchor": "AnchorString"}, "anchor string format")

        # 验证格式 2: 字符串数组 (解析时直接设置为当前节点名)
        obj2 = new_ctx.get_node_object("AnchorArray")
        assert_not_none(obj2, "AnchorArray should exist")
        assert_eq(obj2.anchor, {"ArrayAnchor1": "AnchorArray", "ArrayAnchor2": "AnchorArray"}, "anchor array format")

        # 验证格式 3: 对象
        obj3 = new_ctx.get_node_object("AnchorObject")
        assert_not_none(obj3, "AnchorObject should exist")
        assert_eq(obj3.anchor.get("ObjAnchor1"), "TargetNode1", "anchor object mapping 1")
        assert_eq(obj3.anchor.get("ObjAnchor2"), "", "anchor object mapping 2 (empty = clear)")
        assert_eq(obj3.anchor.get("ObjAnchor3"), "TargetNode2", "anchor object mapping 3")

        print("    PASS: anchor object format")


    def _test_v2_format(self, context: Context):
        print("  Testing v2 format parsing...")

        new_ctx = context.clone()

        # v2 格式
        new_ctx.override_pipeline(
            {
                "V2FormatNode": {
                    "recognition": {
                        "type": "TemplateMatch",
                        "param": {
                            "template": ["v2.png"],
                            "threshold": 0.9,
                            "roi": [0, 0, 100, 100],
                        },
                    },
                    "action": {
                        "type": "Click",
                        "param": {"target": True, "contact": 2},
                    },
                    "pre_delay": 50,
                    "post_delay": 150,
                }
            }
        )

        obj = new_ctx.get_node_object("V2FormatNode")

        assert_eq(obj.recognition.type, JRecognitionType.TemplateMatch, "v2 reco type")
        assert_eq(obj.recognition.param.template, ["v2.png"], "v2 template")
        assert_eq(obj.recognition.param.threshold, [0.9], "v2 threshold")

        assert_eq(obj.action.type, JActionType.Click, "v2 action type")
        assert_eq(obj.action.param.contact, 2, "v2 contact")

        assert_eq(obj.pre_delay, 50, "v2 pre_delay")
        assert_eq(obj.post_delay, 150, "v2 post_delay")

        print("    PASS: v2 format parsing")

    def _test_context_vs_resource(self, context: Context):
        print("  Testing Context vs Resource get_node...")

        # 从 Resource 获取
        res_data = context.tasker.resource.get_node_data("TestBasic")
        res_obj = context.tasker.resource.get_node_object("TestBasic")

        # 从 Context 获取（未 override 时应该相同）
        ctx_data = context.get_node_data("TestBasic")
        ctx_obj = context.get_node_object("TestBasic")

        assert_eq(res_data, ctx_data, "resource and context node_data should match")
        assert_eq(res_obj.recognition.type, ctx_obj.recognition.type, "reco type match")
        assert_eq(res_obj.action.type, ctx_obj.action.type, "action type match")

        # override 后 Context 应该返回新数据，Resource 不变
        new_ctx = context.clone()
        new_ctx.override_pipeline({"TestBasic": {"action": "Click", "timeout": 99999}})

        new_ctx_obj = new_ctx.get_node_object("TestBasic")
        res_obj_after = context.tasker.resource.get_node_object("TestBasic")

        assert_eq(
            new_ctx_obj.action.type, JActionType.Click, "context overridden action"
        )
        assert_eq(new_ctx_obj.timeout, 99999, "context overridden timeout")
        assert_eq(
            res_obj_after.action.type, JActionType.DoNothing, "resource unchanged"
        )

        print("    PASS: Context vs Resource")


class PipelineTestAction(CustomAction):
    def run(
        self, context: Context, argv: CustomAction.RunArg
    ) -> CustomAction.RunResult:
        return CustomAction.RunResult(success=True)


# ============================================================================
# wait_freezes 测试
# ============================================================================


def test_wait_freezes(context: Context):
    """测试 wait_freezes 参数解析"""
    print("\n=== test_wait_freezes ===")

    new_ctx = context.clone()

    # 简单数值格式
    new_ctx.override_pipeline(
        {"WaitFreezesSimple": {"pre_wait_freezes": 500, "post_wait_freezes": 1000}}
    )

    obj = new_ctx.get_node_object("WaitFreezesSimple")
    assert_eq(obj.pre_wait_freezes.time, 500, "pre_wait_freezes.time")
    assert_eq(obj.post_wait_freezes.time, 1000, "post_wait_freezes.time")

    # 对象格式
    new_ctx.override_pipeline(
        {
            "WaitFreezesObject": {
                "pre_wait_freezes": {
                    "time": 800,
                    "target": [100, 100, 200, 200],
                    "threshold": 0.98,
                    "method": 3,
                    "rate_limit": 500,
                    "timeout": 10000,
                }
            }
        }
    )

    obj = new_ctx.get_node_object("WaitFreezesObject")
    assert_eq(obj.pre_wait_freezes.time, 800, "pre_wait_freezes.time")
    assert_eq(obj.pre_wait_freezes.threshold, 0.98, "threshold")
    assert_eq(obj.pre_wait_freezes.method, 3, "method")
    assert_eq(obj.pre_wait_freezes.rate_limit, 500, "rate_limit")
    assert_eq(obj.pre_wait_freezes.timeout, 10000, "timeout")

    print("  PASS: wait_freezes")


# ============================================================================
# repeat 参数测试
# ============================================================================


def test_repeat_params(context: Context):
    """测试 repeat 相关参数"""
    print("\n=== test_repeat_params ===")

    new_ctx = context.clone()

    new_ctx.override_pipeline(
        {
            "RepeatTest": {
                "repeat": 5,
                "repeat_delay": 300,
                "repeat_wait_freezes": {"time": 200, "threshold": 0.9},
            }
        }
    )

    obj = new_ctx.get_node_object("RepeatTest")
    assert_eq(obj.repeat, 5, "repeat")
    assert_eq(obj.repeat_delay, 300, "repeat_delay")
    assert_eq(obj.repeat_wait_freezes.time, 200, "repeat_wait_freezes.time")
    assert_eq(obj.repeat_wait_freezes.threshold, 0.9, "repeat_wait_freezes.threshold")

    print("  PASS: repeat params")


# ============================================================================
# 负数 roi 和 target 参数测试
# ============================================================================


def test_negative_roi_and_target(context: Context):
    """测试负数 roi 和 target 参数的解析"""
    print("\n=== test_negative_roi_and_target ===")

    new_ctx = context.clone()

    # 测试负数 roi 坐标（从边缘反向计算）
    new_ctx.override_pipeline(
        {
            "NegativeRoiCoord": {
                "recognition": "TemplateMatch",
                "template": ["test.png"],
                "roi": [-100, -100, 50, 50],
            }
        }
    )
    obj = new_ctx.get_node_object("NegativeRoiCoord")
    assert_eq(obj.recognition.param.roi, [-100, -100, 50, 50], "negative roi coords")

    # 测试负数 roi 宽高（xy 作为右下角）
    new_ctx.override_pipeline(
        {
            "NegativeRoiSize": {
                "recognition": "TemplateMatch",
                "template": ["test.png"],
                "roi": [200, 200, -100, -50],
            }
        }
    )
    obj = new_ctx.get_node_object("NegativeRoiSize")
    assert_eq(obj.recognition.param.roi, [200, 200, -100, -50], "negative roi size")

    # 测试 roi 宽高为 0（延伸至边缘）
    new_ctx.override_pipeline(
        {
            "ZeroRoiSize": {
                "recognition": "TemplateMatch",
                "template": ["test.png"],
                "roi": [100, 100, 0, 0],
            }
        }
    )
    obj = new_ctx.get_node_object("ZeroRoiSize")
    assert_eq(obj.recognition.param.roi, [100, 100, 0, 0], "zero roi size")

    # 测试组合：负数坐标 + 零宽高（右下角到边缘）
    new_ctx.override_pipeline(
        {
            "NegativeRoiCombo": {
                "recognition": "TemplateMatch",
                "template": ["test.png"],
                "roi": [-100, -100, 0, 0],
            }
        }
    )
    obj = new_ctx.get_node_object("NegativeRoiCombo")
    assert_eq(obj.recognition.param.roi, [-100, -100, 0, 0], "negative roi combo")

    # 测试 2 元素数组的负数坐标
    new_ctx.override_pipeline(
        {
            "NegativeRoi2Elem": {
                "recognition": "TemplateMatch",
                "template": ["test.png"],
                "roi": [-50, -50],
            }
        }
    )
    obj = new_ctx.get_node_object("NegativeRoi2Elem")
    assert_eq(obj.recognition.param.roi, [-50, -50, 0, 0], "negative roi 2-element")

    # 测试负数 target 坐标
    new_ctx.override_pipeline(
        {
            "NegativeTargetCoord": {
                "action": "Click",
                "target": [-100, -100, 50, 50],
            }
        }
    )
    obj = new_ctx.get_node_object("NegativeTargetCoord")
    assert_eq(obj.action.param.target, [-100, -100, 50, 50], "negative target coords")

    # 测试负数 target 宽高
    new_ctx.override_pipeline(
        {
            "NegativeTargetSize": {
                "action": "Click",
                "target": [300, 300, -100, -100],
            }
        }
    )
    obj = new_ctx.get_node_object("NegativeTargetSize")
    assert_eq(obj.action.param.target, [300, 300, -100, -100], "negative target size")

    # 测试 2 元素数组的负数 target
    new_ctx.override_pipeline(
        {
            "NegativeTarget2Elem": {
                "action": "Click",
                "target": [-50, -50],
            }
        }
    )
    obj = new_ctx.get_node_object("NegativeTarget2Elem")
    assert_eq(obj.action.param.target, [-50, -50, 0, 0], "negative target 2-element")

    # 测试 Swipe 的负数坐标
    new_ctx.override_pipeline(
        {
            "NegativeSwipe": {
                "action": "Swipe",
                "begin": [-100, -100, 50, 50],
                "end": [-50, -50],
            }
        }
    )
    obj = new_ctx.get_node_object("NegativeSwipe")
    assert_eq(obj.action.param.begin, [-100, -100, 50, 50], "negative swipe begin")
    assert_eq(obj.action.param.end[0], [-50, -50, 0, 0], "negative swipe end")

    print("  PASS: negative roi and target")


# ============================================================================
# 主测试流程
# ============================================================================


def create_test_pipeline_resource(resource_dir: Path):
    """创建测试用的 pipeline 资源"""
    pipeline_dir = resource_dir / "pipeline"
    pipeline_dir.mkdir(parents=True, exist_ok=True)

    # 创建测试 pipeline JSON
    test_pipeline = {
        "TestBasic": {
            # 使用默认值
        },
        "TestEntry": {
            "next": ["TestReco"],
        },
        "TestReco": {
            "recognition": "Custom",
            "custom_recognition": "PipelineTestReco",
            "action": "Custom",
            "custom_action": "PipelineTestAct",
        },
    }

    with open(pipeline_dir / "test.json", "w", encoding="utf-8") as f:
        json.dump(test_pipeline, f, indent=4)


def main():
    print(f"MaaFw Version: {Library.version()}")
    Toolkit.init_option(install_dir / "bin")

    # 重置测试状态，避免同一进程多次运行时残留陈旧状态
    PipelineTestRecognition.test_results.clear()

    # 创建临时测试资源目录
    import tempfile

    with tempfile.TemporaryDirectory() as tmp_dir:
        test_resource_dir = Path(tmp_dir) / "resource"
        create_test_pipeline_resource(test_resource_dir)

        # 创建 Resource 并加载
        resource = Resource()
        resource.post_bundle(str(test_resource_dir)).wait()

        if not resource.loaded:
            print("Failed to load resource")
            sys.exit(1)

        # 注册自定义识别和动作
        resource.register_custom_recognition(
            "PipelineTestReco", PipelineTestRecognition()
        )
        resource.register_custom_action("PipelineTestAct", PipelineTestAction())

        # Resource 级别测试
        test_resource_get_node_data(resource)
        test_resource_get_node_object(resource)
        test_resource_node_list(resource)

        # 创建 Controller 和 Tasker 进行 Context 级别测试
        dbg_controller = DbgController(
            install_dir / "test" / "PipelineSmoking" / "Screenshot",
            install_dir / "test" / "user",
            MaaDbgControllerTypeEnum.CarouselImage,
        )
        dbg_controller.post_connection().wait()

        tasker = Tasker()
        tasker.bind(resource, dbg_controller)

        if not tasker.inited:
            print("Failed to init tasker")
            sys.exit(1)

        # 运行任务触发 Context 级别测试
        detail = tasker.post_task("TestEntry", {}).wait().get()
        if not detail:
            print("Pipeline task failed")
            sys.exit(1)

        # 检查 Context 测试结果
        if not PipelineTestRecognition.test_results.get("context_tests"):
            print("Context tests failed")
            sys.exit(1)

        # 额外的 Context 级别测试（在任务外进行）
        # 这些测试需要通过创建一个简单的任务来获取 Context
        print("\n=== Additional Context-level tests ===")

        # 创建一个新任务来测试更多场景
        class AdditionalTestReco(CustomRecognition):
            def analyze(self, context, argv):
                test_wait_freezes(context)
                test_repeat_params(context)
                test_negative_roi_and_target(context)
                return CustomRecognition.AnalyzeResult(
                    box=(0, 0, 10, 10), detail="done"
                )

        resource.register_custom_recognition("AdditionalTestReco", AdditionalTestReco())

        detail2 = (
            tasker.post_task(
                "AdditionalTest",
                {
                    "AdditionalTest": {
                        "recognition": "Custom",
                        "custom_recognition": "AdditionalTestReco",
                    }
                },
            )
            .wait()
            .get()
        )

        if not detail2:
            print("Additional tests task failed")
            sys.exit(1)

    print("\n" + "=" * 50)
    print("All pipeline tests passed!")
    print("=" * 50)


if __name__ == "__main__":
    main()
