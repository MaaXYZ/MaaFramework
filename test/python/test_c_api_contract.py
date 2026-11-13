"""Exercise C API validation paths that public Python wrappers cannot express."""

import ctypes
import unittest

from maa.buffer import (
    ImageBuffer,
    ImageListBuffer,
    RectBuffer,
    StringBuffer,
    StringListBuffer,
)
from maa.context import Context
from maa.controller import Controller
from maa.define import (
    MaaBool,
    MaaImageBufferHandle,
    MaaSize,
    MaaStatus,
    MaaTaskerHandle,
)
from maa.library import Library
from maa.resource import Resource
from maa.tasker import Tasker
from test_api_contract import CallbackAction, RecordingController


NULL_HANDLE_APIS = {
    "StringBuffer": "Destroy IsEmpty Clear Get Size Set SetEx",
    "StringListBuffer": "Destroy IsEmpty Clear Size At Append Remove",
    "ImageBuffer": (
        "Destroy IsEmpty Clear GetRawData Width Height Channels Type SetRawData "
        "Resize GetEncoded GetEncodedSize SetEncoded"
    ),
    "ImageListBuffer": "Destroy IsEmpty Clear Size At Append Remove",
    "Rect": "Destroy GetX GetY GetW GetH Set",
    "Resource": (
        "Destroy AddSink RemoveSink ClearSinks RegisterCustomRecognition "
        "UnregisterCustomRecognition ClearCustomRecognition RegisterCustomAction "
        "UnregisterCustomAction ClearCustomAction PostBundle PostOcrModel PostPipeline "
        "PostImage OverridePipeline OverrideNext OverrideImage GetNodeData Clear Status "
        "Wait Loaded SetOption GetLoadDetail GetHash GetNodeList GetCustomRecognitionList "
        "GetCustomActionList GetDefaultRecognitionParam GetDefaultActionParam"
    ),
    "Controller": (
        "Destroy AddSink RemoveSink ClearSinks SetOption PostConnection PostClick "
        "PostClickV2 PostSwipe PostSwipeV2 PostClickKey PostInputText PostStartApp "
        "PostStopApp PostTouchDown PostTouchMove PostTouchUp PostRelativeMove "
        "PostKeyDown PostKeyUp PostScroll PostShell GetShellOutput PostScreencap "
        "PostInactive Status Wait Connected CachedImage GetUuid GetResolution GetInfo"
    ),
    "Tasker": (
        "Destroy AddSink RemoveSink ClearSinks AddContextSink RemoveContextSink "
        "ClearContextSinks SetOption BindResource BindController Inited PostTask "
        "PostRecognition PostAction Status Wait Running PostStop Stopping GetResource "
        "GetController ClearCache OverridePipeline GetRecognitionDetail GetActionDetail "
        "GetWaitFreezesDetail GetNodeDetail GetTaskDetail GetLatestNode"
    ),
    "Context": (
        "RunTask RunRecognition RunAction RunRecognitionDirect RunActionDirect "
        "WaitFreezes OverridePipeline OverrideNext OverrideImage GetNodeData GetTaskId "
        "GetTasker Clone SetAnchor GetAnchor GetHitCount ClearHitCount"
    ),
}


def null_argument(argtype):
    if argtype is ctypes.c_char_p:
        return b""
    if issubclass(argtype, ctypes._CFuncPtr):
        return argtype()
    return argtype()


class NullHandleTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        for wrapper in (
            StringBuffer,
            StringListBuffer,
            ImageBuffer,
            ImageListBuffer,
            RectBuffer,
            Resource,
            Controller,
            Tasker,
            Context,
        ):
            wrapper._set_api_properties()
        cls.lib = Library.framework()
        # These C entry points have no public Python wrappers declaring their ABI.
        encoded = ctypes.POINTER(ctypes.c_uint8)
        for name, result, arguments in (
            ("MaaImageBufferGetEncoded", encoded, [MaaImageBufferHandle]),
            ("MaaImageBufferGetEncodedSize", MaaSize, [MaaImageBufferHandle]),
            (
                "MaaImageBufferSetEncoded",
                MaaBool,
                [MaaImageBufferHandle, encoded, MaaSize],
            ),
            (
                "MaaTaskerSetOption",
                MaaBool,
                [MaaTaskerHandle, ctypes.c_int32, ctypes.c_void_p, MaaSize],
            ),
        ):
            function = getattr(cls.lib, name)
            function.restype = result
            function.argtypes = arguments


def null_handle_test(name):
    def test(self):
        function = getattr(self.lib, name)
        self.assertIsNotNone(function.argtypes, f"{name} must declare ctypes arguments")
        arguments = [null_argument(argtype) for argtype in function.argtypes]
        variants = [("empty_strings", arguments)]
        for index, argtype in enumerate(function.argtypes):
            if argtype is ctypes.c_char_p:
                null_string = arguments.copy()
                null_string[index] = None
                variants.append((f"null_string_{index}", null_string))
        for variant, values in variants:
            with self.subTest(variant=variant):
                result = function(*values)
                if name.endswith("IsEmpty"):
                    self.assertTrue(result, "a null buffer is empty")
                else:
                    self.assertFalse(result, f"{name} must reject a null handle")

    return test


for module, actions in NULL_HANDLE_APIS.items():
    for action in actions.split():
        api_name = f"Maa{module}{action}"
        setattr(NullHandleTest, f"test_null_{api_name}", null_handle_test(api_name))


class ResourceInputValidationTest(unittest.TestCase):
    def setUp(self):
        self.resource = Resource()
        self.lib = Library.framework()
        self.handle = self.resource._handle

    def test_null_paths_are_rejected_without_enqueueing(self):
        for action in ("PostBundle", "PostOcrModel", "PostPipeline", "PostImage"):
            with self.subTest(action=action):
                self.assertEqual(getattr(self.lib, "MaaResource" + action)(self.handle, None), 0)

    def test_invalid_json_is_rejected_and_resource_remains_usable(self):
        for text in (None, b"", b"{", b"null", b"[]", b"1", b'"node"'):
            with self.subTest(text=text):
                self.assertFalse(self.lib.MaaResourceOverridePipeline(self.handle, text))
        self.assertTrue(self.resource.override_pipeline({"Valid": {}}))
        self.assertIsNotNone(self.resource.get_node_data("Valid"))

    def test_null_output_buffers_are_rejected(self):
        for action in (
            "GetHash",
            "GetNodeList",
            "GetCustomRecognitionList",
            "GetCustomActionList",
        ):
            with self.subTest(action=action):
                self.assertFalse(getattr(self.lib, "MaaResource" + action)(self.handle, None))
        self.assertFalse(self.lib.MaaResourceGetNodeData(self.handle, b"Missing", None))
        self.assertFalse(self.lib.MaaResourceGetDefaultRecognitionParam(self.handle, b"OCR", None))
        self.assertFalse(self.lib.MaaResourceGetDefaultActionParam(self.handle, b"Click", None))

    def test_failed_node_query_preserves_output_buffer(self):
        output = StringBuffer()
        self.assertTrue(output.set("sentinel"))
        for node_name in (None, b"Missing"):
            with self.subTest(node_name=node_name):
                self.assertFalse(self.lib.MaaResourceGetNodeData(self.handle, node_name, output._handle))
                self.assertEqual(output.get(), "sentinel")

    def test_null_override_values_are_rejected(self):
        self.assertFalse(self.lib.MaaResourceOverrideNext(self.handle, b"Missing", None))
        self.assertFalse(self.lib.MaaResourceOverrideImage(self.handle, b"image.png", None))


class BoundTaskerTest(unittest.TestCase):
    def setUp(self):
        self.resource = Resource()
        self.assertTrue(self.resource.override_pipeline({"Entry": {"pre_delay": 0, "post_delay": 0}}))
        self.controller = RecordingController()
        self.assertTrue(self.controller.post_connection().wait().succeeded)
        self.tasker = Tasker()
        self.assertTrue(self.tasker.bind(self.resource, self.controller))
        self.assertTrue(self.tasker.inited)
        self.lib = Library.framework()
        self.handle = self.tasker._handle

    def tearDown(self):
        self.tasker.post_stop().wait()
        self.resource.clear_custom_action()
        self.resource.clear_custom_recognition()


class TaskerInputValidationTest(BoundTaskerTest):
    def test_null_entry_and_override_are_rejected(self):
        for entry, override in ((None, b"{}"), (b"Entry", None)):
            with self.subTest(entry=entry, override=override):
                self.assertEqual(self.lib.MaaTaskerPostTask(self.handle, entry, override), 0)

    def test_invalid_task_override_is_rejected(self):
        for override in (b"", b"{", b"null", b"1", b"[1]"):
            with self.subTest(override=override):
                self.assertEqual(self.lib.MaaTaskerPostTask(self.handle, b"Entry", override), 0)
        self.assertFalse(self.tasker.running)

    def test_empty_override_array_is_a_valid_noop(self):
        task_id = self.lib.MaaTaskerPostTask(self.handle, b"Entry", b"[]")
        self.assertNotEqual(task_id, 0)
        self.lib.MaaTaskerWait(self.handle, task_id)
        detail = self.tasker.get_task_detail(task_id)
        self.assertIsNotNone(detail)
        self.assertTrue(detail.status.succeeded)
        self.assertEqual([node.name for node in detail.nodes], ["Entry"])

    def test_direct_calls_reject_null_input(self):
        image = ImageBuffer()
        rect = RectBuffer()
        for kind, param, image_handle in (
            (None, b"{}", image._handle),
            (b"DirectHit", None, image._handle),
            (b"DirectHit", b"{}", None),
        ):
            with self.subTest(kind=kind, param=param, image=image_handle):
                self.assertEqual(
                    self.lib.MaaTaskerPostRecognition(self.handle, kind, param, image_handle),
                    0,
                )
        for kind, param, box, detail in (
            (None, b"{}", rect._handle, b""),
            (b"DoNothing", None, rect._handle, b""),
            (b"DoNothing", b"{}", None, b""),
            (b"DoNothing", b"{}", rect._handle, None),
        ):
            with self.subTest(kind=kind, param=param, box=box, detail=detail):
                self.assertEqual(
                    self.lib.MaaTaskerPostAction(self.handle, kind, param, box, detail),
                    0,
                )

    def test_recognition_node_rejects_invalid_arguments(self):
        image = ImageBuffer()
        for handle, entry, override, pixels in (
            (None, b"Entry", b"{}", image._handle),
            (self.handle, None, b"{}", image._handle),
            (self.handle, b"Entry", None, image._handle),
            (self.handle, b"Entry", b"{}", None),
            (self.handle, b"Entry", b"{", image._handle),
            (self.handle, b"Entry", b"[1]", image._handle),
        ):
            with self.subTest(entry=entry, override=override, image=pixels):
                self.assertEqual(self.lib.MaaTaskerPostRecognitionNode(handle, entry, override, pixels), 0)

    def test_missing_detail_does_not_modify_output(self):
        name = StringBuffer()
        self.assertTrue(name.set("sentinel"))
        count = MaaSize(123)
        status = MaaStatus(456)
        self.assertFalse(
            self.lib.MaaTaskerGetTaskDetail(
                self.handle,
                0,
                name._handle,
                None,
                ctypes.byref(count),
                ctypes.byref(status),
            )
        )
        self.assertEqual(name.get(), "sentinel")
        self.assertEqual(count.value, 123)
        self.assertEqual(status.value, 456)


class ControllerInputValidationTest(BoundTaskerTest):
    def test_null_text_is_rejected_before_enqueueing(self):
        for action in ("PostInputText", "PostStartApp", "PostStopApp"):
            with self.subTest(action=action):
                self.assertEqual(
                    getattr(self.lib, "MaaController" + action)(self.controller._handle, None),
                    0,
                )
        self.assertEqual(self.lib.MaaControllerPostShell(self.controller._handle, None, 100), 0)
        self.assertTrue(self.controller.post_input_text("valid").wait().succeeded)


class ContextInputValidationTest(BoundTaskerTest):
    def test_null_required_arguments_are_rejected_inside_callback(self):
        def check(context, argv):
            image, rect, output, names = (
                ImageBuffer(),
                RectBuffer(),
                StringBuffer(),
                StringListBuffer(),
            )
            self.assertTrue(image.set(self.controller.image))
            cases = (
                ("RunTask", [b"Entry", b"{}"], [0, 1]),
                ("RunRecognition", [b"Entry", b"{}", image._handle], [0, 1, 2]),
                ("RunAction", [b"Entry", b"{}", rect._handle, b""], [0, 1, 3]),
                (
                    "RunRecognitionDirect",
                    [b"DirectHit", b"{}", image._handle],
                    [0, 1, 2],
                ),
                (
                    "RunActionDirect",
                    [b"DoNothing", b"{}", rect._handle, b""],
                    [0, 1, 2, 3],
                ),
                ("OverridePipeline", [b"{}"], [0]),
                ("OverrideNext", [b"Entry", names._handle], [0, 1]),
                ("OverrideImage", [b"image.png", image._handle], [0, 1]),
                ("GetNodeData", [b"Entry", output._handle], [0, 1]),
                ("SetAnchor", [b"Anchor", b"Entry"], [0, 1]),
                ("GetAnchor", [b"Anchor", output._handle], [0, 1]),
                ("GetHitCount", [b"Entry", ctypes.pointer(MaaSize())], [0, 1]),
                ("ClearHitCount", [b"Entry"], [0]),
            )
            for action, arguments, required in cases:
                function = getattr(self.lib, "MaaContext" + action)
                for index in required:
                    invalid = arguments.copy()
                    invalid[index] = None
                    self.assertFalse(
                        function(context._handle, *invalid),
                        f"{action} argument {index}",
                    )
            action_id = self.lib.MaaContextRunAction(context._handle, b"Entry", b"{}", None, b"")
            self.assertNotEqual(action_id, 0, "RunAction accepts an omitted recognition box")
            self.assertTrue(context.tasker.get_action_detail(action_id).success)
            self.assertTrue(rect.set([0, 0, 16, 16]))
            self.assertTrue(self.lib.MaaContextWaitFreezes(context._handle, 1, rect._handle, None))
            self.assertTrue(self.lib.MaaContextWaitFreezes(context._handle, 1, None, b'{"target":[0,0,16,16]}'))

        action = CallbackAction(check)
        self.assertTrue(self.resource.register_custom_action("Validate", action))
        job = self.tasker.post_task(
            "Validate",
            {
                "Validate": {
                    "action": "Custom",
                    "custom_action": "Validate",
                    "pre_delay": 0,
                    "post_delay": 0,
                }
            },
        ).wait()
        self.assertEqual(action.calls, 1)
        self.assertIsNone(action.error, action.error)
        self.assertTrue(job.succeeded)


if __name__ == "__main__":
    unittest.main()
