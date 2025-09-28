import os
from pathlib import Path
import sys

import numpy

if len(sys.argv) < 3:
    print("Usage: python binding_test.py <binding_dir> <install_dir>")
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
from maa.controller import DbgController, CustomController
from maa.tasker import Tasker
from maa.toolkit import Toolkit
from maa.custom_action import CustomAction
from maa.custom_recognition import CustomRecognition
from maa.define import RectType, MaaDbgControllerTypeEnum, LoggingLevelEnum
from maa.context import Context
from maa.notification_handler import NotificationHandler

analyzed: bool = False
runned: bool = False


class MyRecognition(CustomRecognition):

    def analyze(
        self,
        context: Context,
        argv: CustomRecognition.AnalyzeArg,
    ) -> CustomRecognition.AnalyzeResult:
        print(
            f"on MyRecognition.analyze, context: {context}, image: {argv.image.shape}, task_detail: {argv.task_detail}, reco_name: {argv.custom_recognition_name}, reco_param: {argv.custom_recognition_param}"
        )
        entry = "ColorMatch"
        ppover = {
            "ColorMatch": {
                "recognition": "ColorMatch",
                "lower": [100, 100, 100],
                "upper": [255, 255, 255],
                "action": "Click",
            }
        }
        context.run_task(entry, ppover)
        context.run_action(entry, [114, 514, 191, 810], "RunAction Detail", ppover)
        reco_detail = context.run_recognition(entry, argv.image, ppover)
        print(f"reco_detail: {reco_detail}")

        new_ctx = context.clone()
        new_ctx.override_pipeline({"TaskA": {}, "TaskB": {}})
        new_ctx.override_next(argv.node_name, ["TaskA", "TaskB"])

        node_detail = new_ctx.tasker.get_latest_node("ColorMatch")
        print(node_detail)

        task_job = new_ctx.get_task_job()
        new_task_detail = task_job.get()
        print(new_task_detail)

        global analyzed
        analyzed = True

        return CustomRecognition.AnalyzeResult(
            box=(11, 4, 5, 14), detail="Hello World!"
        )


class MyAction(CustomAction):
    def run(
        self,
        context: Context,
        argv: CustomAction.RunArg,
    ) -> CustomAction.RunResult:
        print(
            f"on MyAction.run, context: {context}, task_detail: {argv.task_detail}, action_name: {argv.custom_action_name}, action_param: {argv.custom_action_param}, box: {argv.box}, reco_detail: {argv.reco_detail}"
        )
        controller = context.tasker.controller
        new_image = controller.post_screencap().wait().get()
        print(f"new_image: {new_image.shape}")
        controller.post_click(191, 98).wait()
        controller.post_swipe(100, 200, 300, 400, 100).wait()
        controller.post_input_text("Hello World!").wait()
        controller.post_click_key(32).wait()
        controller.post_touch_down(1, 100, 100, 0).wait()
        controller.post_touch_move(1, 200, 200, 0).wait()
        controller.post_touch_up(1).wait()
        controller.post_start_app("aaa")
        controller.post_stop_app("bbb")

        cached_image = controller.cached_image
        connected = controller.connected
        uuid = controller.uuid
        controller.set_screenshot_target_long_side(1080)
        controller.set_screenshot_target_short_side(720)

        global runned
        runned = True

        return CustomAction.RunResult(success=True)


def api_test():
    r1 = Resource()
    r1.use_directml()
    r1.use_coreml()
    r1.use_auto_ep()
    r2 = Resource()
    r2.use_cpu()
    r2.use_directml(114514)
    r2.use_cpu()
    r2.post_bundle("C:/_maafw_testing_/aaabbbccc").wait()
    t1 = Tasker()
    t2 = Tasker()
    t2.post_task("Entry", {}).wait()

    resource = Resource(MyNotificationHandler())
    print(f"resource: {resource}")

    dbg_controller = DbgController(
        install_dir / "test" / "PipelineSmoking" / "Screenshot",
        install_dir / "test" / "user",
        MaaDbgControllerTypeEnum.CarouselImage,
        notification_handler=MyNotificationHandler(),
    )
    print(f"controller: {dbg_controller}")
    dbg_controller.post_connection().wait()

    tasker = Tasker(notification_handler=MyNotificationHandler())
    tasker.bind(resource, dbg_controller)
    print(f"tasker: {tasker}")

    if not tasker.inited:
        print("failed to init tasker")
        exit(1)

    resource.register_custom_action("MyAct", MyAction())
    resource.register_custom_recognition("MyRec", MyRecognition())

    ppover = {
        "Entry": {"next": "Rec"},
        "Rec": {
            "recognition": "Custom",
            "custom_recognition": "MyRec",
            "action": "Custom",
            "custom_action": "MyAct",
            "custom_action_param": "Test111222333",
        },
    }

    detail = tasker.post_task("Entry", ppover).wait().get()
    if detail:
        print(f"pipeline detail: {detail}")
    else:
        print("pipeline failed")
        raise RuntimeError("pipeline failed")

    stopping = tasker.stopping
    print(f"stopping0: {stopping}")

    tasker.post_task("Entry", ppover)

    stop_job = tasker.post_stop()
    stopping = tasker.stopping

    print(f"stopping1: {stopping}")
    if not stop_job.wait().succeeded:
        raise RuntimeError("post_stop failed")

    stopping = tasker.stopping
    print(f"stopping2: {stopping}")

    tasker.resource.post_bundle("C:/_maafw_testing_/aaabbbccc")
    print(f"node_list: {tasker.resource.node_list}")
    tasker.clear_cache()
    inited = tasker.inited
    running = tasker.running

    Tasker.set_save_draw(True)
    Tasker.set_stdout_level(LoggingLevelEnum.All)
    Tasker.set_log_dir(".")

    devices = Toolkit.find_adb_devices()
    print(f"devices: {devices}")
    desktop = Toolkit.find_desktop_windows()
    print(f"desktop: {desktop}")
    Toolkit.pi_register_custom_action("MyAct", MyAction())
    Toolkit.pi_register_custom_recognition("MyRec", MyRecognition())
    # Toolkit.run_pi_cli("C:/_maafw_testing_/aaabbbccc", ".", True)

    # Test parse_pipeline_data and dump_pipeline_data functions
    pipeline_data_test(tasker)

    global analyzed, runned
    if not analyzed or not runned:
        print("failed to run custom recognition or action")
        raise RuntimeError("failed to run custom recognition or action")



def pipeline_data_test(tasker: Tasker):
    """Test parse_pipeline_data and dump_pipeline_data round-trip conversion."""
    print("test_pipeline_data")
    
    # Use the resource directly since it has both override_pipeline and get_node_data
    resource = tasker.resource
    
    # Test 1: Test direct conversion functions first (most reliable)
    print("  Round 1: Testing direct conversion functions...")
    from maa.resource import parse_pipeline_data, dump_pipeline_data
    import json
    
    # Test data with various field types - single node data
    test_node_data = {
        "recognition": {
            "type": "TemplateMatch",
            "param": {
                "template": ["test.png", "test2.png"],
                "roi": [0, 0, 100, 100],
                "roi_offset": [5, 5, -5, -5],
                "threshold": [0.8, 0.9],
                "order_by": "score",
                "index": 1,
                "method": 3,
                "green_mask": True
            }
        },
        "action": {
            "type": "Click",
            "param": {
                "target": [50, 50, 20, 20],
                "target_offset": [2, 2, 0, 0]
            }
        },
        "next": ["NextTask", "FallbackTask"],
        "interrupt": ["CancelTask"],
        "timeout": 5000,
        "enabled": True,
        "pre_delay": 100,
        "post_delay": 200
    }
    
    # Convert dict to JSON string and parse
    json_str = json.dumps(test_node_data, ensure_ascii=False)
    parsed_data = parse_pipeline_data(json_str)
    
    print(f"  Parsed recognition type: {parsed_data.recognition.type}")
    print(f"  Parsed action type: {parsed_data.action.type}")
    print(f"  Parsed next tasks: {parsed_data.next}")
    print(f"  Parsed timeout: {parsed_data.timeout}")
    
    # Convert back to JSON string
    dumped_json = dump_pipeline_data(parsed_data)
    print(f"  Dumped JSON length: {len(dumped_json)} characters")
    
    # Parse again to verify round-trip
    reparsed_data = parse_pipeline_data(dumped_json)
    
    conversion_match = True
    if parsed_data.recognition.type != reparsed_data.recognition.type:
        print(f"  ERROR: Recognition type mismatch: {parsed_data.recognition.type} != {reparsed_data.recognition.type}")
        conversion_match = False
        
    if parsed_data.action.type != reparsed_data.action.type:
        print(f"  ERROR: Action type mismatch: {parsed_data.action.type} != {reparsed_data.action.type}")
        conversion_match = False
        
    if parsed_data.next != reparsed_data.next:
        print(f"  ERROR: Next tasks mismatch: {parsed_data.next} != {reparsed_data.next}")
        conversion_match = False
        
    if parsed_data.timeout != reparsed_data.timeout:
        print(f"  ERROR: Timeout mismatch: {parsed_data.timeout} != {reparsed_data.timeout}")
        conversion_match = False
    
    # Test 2: Test with simpler data
    print("  Round 2: Testing with simple DirectHit + DoNothing...")
    simple_node_data = {
        "recognition": {"type": "DirectHit", "param": {}},
        "action": {"type": "DoNothing", "param": {}},
        "next": ["Task1"],
        "enabled": True
    }
    
    simple_json = json.dumps(simple_node_data, ensure_ascii=False)
    simple_parsed = parse_pipeline_data(simple_json)
    simple_dumped = dump_pipeline_data(simple_parsed)
    simple_reparsed = parse_pipeline_data(simple_dumped)
    
    simple_match = True
    if simple_parsed.recognition.type != "DirectHit":
        print(f"  ERROR: Simple recognition type mismatch: {simple_parsed.recognition.type}")
        simple_match = False
    if simple_parsed.action.type != "DoNothing":
        print(f"  ERROR: Simple action type mismatch: {simple_parsed.action.type}")
        simple_match = False
    if simple_parsed.next != simple_reparsed.next:
        print(f"  ERROR: Simple next tasks mismatch: {simple_parsed.next} != {simple_reparsed.next}")
        simple_match = False
    
    # Test 3: Test field mapping (template -> template_)
    print("  Round 3: Testing field mapping...")
    template_data = {
        "recognition": {
            "type": "TemplateMatch",
            "param": {
                "template": ["field_mapping_test.png"],
                "roi": [0, 0, 50, 50],
                "threshold": [0.7]
            }
        },
        "action": {"type": "DoNothing", "param": {}}
    }
    
    template_json = json.dumps(template_data, ensure_ascii=False)
    template_parsed = parse_pipeline_data(template_json)
    template_dumped = dump_pipeline_data(template_parsed)
    
    # Check if template_ appears in dumped JSON (C++ field name)
    field_mapping_match = "template_" in template_dumped
    if not field_mapping_match:
        print(f"  ERROR: Field mapping failed - 'template_' not found in dumped JSON")
    else:
        print("  ✓ Field mapping test passed (template -> template_)")
    
    # Test 4: Test type safety with recognition param variants
    print("  Round 4: Testing recognition parameter variants...")
    variants_match = True
    
    # Check that the parsed param is the correct type
    if hasattr(template_parsed.recognition.param, 'template'):
        print("  ✓ JTemplateMatch param has 'template' attribute")
    else:
        print("  ERROR: JTemplateMatch param missing 'template' attribute")
        variants_match = False
        
    if hasattr(simple_parsed.recognition.param, '__class__'):
        param_class_name = simple_parsed.recognition.param.__class__.__name__
        if param_class_name == 'JDirectHit':
            print("  ✓ JDirectHit param has correct type")
        else:
            print(f"  ERROR: DirectHit param has wrong type: {param_class_name}")
            variants_match = False
    
    # Final result
    if conversion_match and simple_match and field_mapping_match and variants_match:
        print("  ✓ All pipeline data tests passed!")
        return True
    else:
        print("  ✗ Pipeline data tests failed!")
        return False


def custom_ctrl_test():
    print("test_custom_controller")

    controller = MyController(MyNotificationHandler())
    ret = controller.post_connection().wait().succeeded
    uuid = controller.uuid
    ret &= controller.post_start_app("custom_aaa").wait().succeeded
    ret &= controller.post_stop_app("custom_bbb").wait().succeeded
    image_job = controller.post_screencap().wait()
    ret &= image_job.succeeded
    print(f"image: {image_job.get().shape}")
    ret &= controller.post_click(100, 200).wait().succeeded
    ret &= controller.post_swipe(100, 200, 300, 400, 200).wait().succeeded
    ret &= controller.post_touch_down(1, 100, 100, 0).wait().succeeded
    ret &= controller.post_touch_move(1, 200, 200, 0).wait().succeeded
    ret &= controller.post_touch_up(1).wait().succeeded
    ret &= controller.post_click_key(32).wait().succeeded
    ret &= controller.post_input_text("Hello World!").wait().succeeded

    print(f"controller.count: {controller.count}, ret: {ret}")
    # if controller.count != 11 or not ret:
    #     print("failed to run custom controller")
    #     raise RuntimeError("failed to run custom controller")


class MyNotificationHandler(NotificationHandler):
    def on_raw_notification(self, msg: str, details: dict):
        print(
            f"on MyNotificationHandler.on_raw_notification, msg: {msg}, details: {details}"
        )

        super().on_raw_notification(msg, details)


class MyController(CustomController):

    def __init__(self, notification_handler: NotificationHandler):
        super().__init__(notification_handler=notification_handler)
        self.count = 0

    def connect(self) -> bool:
        print("on MyController.connect")
        self.count += 1
        return True

    def request_uuid(self) -> str:
        print("on MyController.request_uuid")
        # self.count += 1
        return "12345678"

    def start_app(self, intent: str) -> bool:
        print(f"on MyController.start_app, intent: {intent}")
        self.count += 1
        return True

    def stop_app(self, intent: str) -> bool:
        print(f"on MyController.stop_app, intent: {intent}")
        self.count += 1
        return True

    def screencap(self) -> numpy.ndarray:
        print("on MyController.screencap")
        self.count += 1
        return numpy.zeros((1080, 1920, 3), dtype=numpy.uint8)

    def click(self, x: int, y: int) -> bool:
        print(f"on MyController.click, x: {x}, y: {y}")
        self.count += 1
        return True

    def swipe(self, x1: int, y1: int, x2: int, y2: int, duration: int) -> bool:
        print(
            f"on MyController.swipe, x1: {x1}, y1: {y1}, x2: {x2}, y2: {y2}, duration: {duration}"
        )
        self.count += 1
        return True

    def touch_down(
        self,
        contact: int,
        x: int,
        y: int,
        pressure: int,
    ) -> bool:
        print(
            f"on MyController.touch_down, contact: {contact}, x: {x}, y: {y}, pressure: {pressure}"
        )
        self.count += 1
        return True

    def touch_move(
        self,
        contact: int,
        x: int,
        y: int,
        pressure: int,
    ) -> bool:
        print(
            f"on MyController.touch_move, contact: {contact}, x: {x}, y: {y}, pressure: {pressure}"
        )
        self.count += 1
        return True

    def touch_up(self, contact: int) -> bool:
        print(f"on MyController.touch_up, contact: {contact}")
        self.count += 1
        return True

    def click_key(self, keycode: int) -> bool:
        print(f"on MyController.click_key, keycode: {keycode}")
        self.count += 1
        return True

    def input_text(self, text: str) -> bool:
        print(f"on MyController.input_text, text: {text}")
        self.count += 1
        return True

    def key_down(self, keycode: int) -> bool:
        print(f"on MyController.key_down, keycode: {keycode}")
        self.count += 1
        return True

    def key_up(self, keycode: int) -> bool:
        print(f"on MyController.key_up, keycode: {keycode}")
        self.count += 1
        return True


if __name__ == "__main__":
    print(f"MaaFw Version: {Library.version()}")

    Toolkit.init_option(install_dir / "bin")

    api_test()
    custom_ctrl_test()
