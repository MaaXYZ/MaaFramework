import os
from pathlib import Path
import sys
import numpy
import io

# Fix encoding issues on Windows (cp1252 cannot encode some Unicode characters)
if sys.stdout.encoding != "utf-8":
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding="utf-8", errors="replace")
if sys.stderr.encoding != "utf-8":
    sys.stderr = io.TextIOWrapper(sys.stderr.buffer, encoding="utf-8", errors="replace")

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
from maa.event_sink import EventSink

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
        action_detail = context.run_action(
            entry, [114, 514, 191, 810], "RunAction Detail", ppover
        )
        print(f"action_detail: {action_detail}")
        reco_detail = context.run_recognition(entry, argv.image, ppover)
        print(f"reco_detail: {reco_detail}")

        new_ctx = context.clone()
        new_ctx.override_pipeline({"TaskA": {}, "TaskB": {}})
        new_ctx.override_next(argv.node_name, ["TaskA", "TaskB"])
        node_data = new_ctx.get_node_data(argv.node_name)
        node_obj = new_ctx.get_node_object(argv.node_name)
        print(f"node_data: {node_data}, node_obj: {node_obj}")

        res_node_data = new_ctx.tasker.resource.get_node_data(argv.node_name)
        res_node_obj = new_ctx.tasker.resource.get_node_object(argv.node_name)
        print(f"res_node_data: {res_node_data}, res_node_obj: {res_node_obj}")

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

    resource = Resource()
    resource.add_sink(MyEventSink())
    print(f"resource: {resource}")

    dbg_controller = DbgController(
        install_dir / "test" / "PipelineSmoking" / "Screenshot",
        install_dir / "test" / "user",
        MaaDbgControllerTypeEnum.CarouselImage,
    )
    print(f"controller: {dbg_controller}")
    dbg_controller.post_connection().wait()

    tasker = Tasker()
    tasker.add_sink(MyEventSink())
    tasker.add_context_sink(MyEventSink())
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

    detail_and = (
        tasker.post_task(
            "and_test",
            {
                "and_test": {
                    "recognition": {
                        "type": "And",
                        "param": {
                            "all_of": [
                                {"recognition": {"type": "DirectHit"}},
                                {"recognition": {"type": "DirectHit"}},
                            ]
                        },
                    },
                }
            },
        )
        .wait()
        .get()
    )
    if detail_and:
        print(f"pipeline detail_and: {detail_and}")
    else:
        print("pipeline detail_and failed")
        raise RuntimeError("pipeline detail_and failed")

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

    global analyzed, runned
    if not analyzed or not runned:
        print("failed to run custom recognition or action")
        raise RuntimeError("failed to run custom recognition or action")


def custom_ctrl_test():
    print("test_custom_controller")

    controller = MyController()
    controller.add_sink(MyEventSink())
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


class MyEventSink(EventSink):
    def _on_raw_notification(self, handle, msg: str, details: dict):
        print(
            f"on MyEventSink._on_raw_notification, handle: {handle}, msg: {msg}, details: {details}"
        )


class MyController(CustomController):

    def __init__(self):
        super().__init__()
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
