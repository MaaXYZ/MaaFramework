from pathlib import Path
import sys

if len(sys.argv) < 2:
    print("Usage: python binding_test.py <install_dir>")
    sys.exit(1)

install_dir = Path(sys.argv[1]).resolve()
print(f"install_dir: {install_dir}")

binding_dir = str(install_dir / "binding" / "Python")
if binding_dir not in sys.path:
    sys.path.insert(0, binding_dir)


from maa.library import Library
from maa.resource import Resource
from maa.controller import DbgController
from maa.tasker import Tasker
from maa.toolkit import Toolkit
from maa.custom_action import CustomAction
from maa.custom_recognizer import CustomRecognizer
from maa.define import RectType, MaaDbgControllerTypeEnum, LoggingLevelEnum
from maa.context import Context

analyzed: bool = False
runned: bool = False


class MyRecognizer(CustomRecognizer):

    def analyze(
        self,
        context: Context,
        argv: CustomRecognizer.AnalyzeArg,
    ) -> CustomRecognizer.AnalyzeResult:
        print(
            f"on MyRecognizer.analyze, context: {context}, image: {argv.image.shape}, task_detail: {argv.task_detail}, reco_name: {argv.custom_recognition_name}, reco_param: {argv.custom_recognition_param}"
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
        context.run_pipeline(entry, ppover)
        context.run_action(entry, [114, 514, 191, 810], "RunAction Detail", ppover)
        reco_detail = context.run_recognition(entry, argv.image, ppover)
        print(f"reco_detail: {reco_detail}")

        new_ctx = context.clone()
        new_ctx.override_next(argv.current_task_name, ["TaskA", "TaskB"])
        new_ctx.override_pipeline({"TaskA": {}, "TaskB": {}})

        node_detail = new_ctx.tasker.get_latest_node("ColorMatch")
        print(node_detail)

        task_job = new_ctx.get_task_job()
        new_task_detail = task_job.get()
        print(new_task_detail)

        global analyzed
        analyzed = True

        return CustomRecognizer.AnalyzeResult(box=(11, 4, 5, 14), detail="Hello World!")


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
        controller.post_press_key(32).wait()
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


def main():
    version = Library.open(install_dir / "bin")
    print(f"MaaFw Version: {version}")

    Toolkit.init_option(install_dir / "bin")

    resource = Resource()
    print(f"resource: {resource}")

    dbg_controller = DbgController(
        install_dir / "test" / "PipelineSmoking" / "Screenshot",
        install_dir / "test" / "user",
        MaaDbgControllerTypeEnum.CarouselImage,
    )
    print(f"controller: {dbg_controller}")
    dbg_controller.post_connection().wait()

    tasker = Tasker()
    tasker.bind(resource, dbg_controller)
    print(f"tasker: {tasker}")

    if not tasker.inited:
        print("failed to init tasker")
        exit(1)

    resource.register_custom_action("MyAct", MyAction())
    resource.register_custom_recognizer("MyRec", MyRecognizer())

    ppover = {
        "Entry": {"next": "Rec"},
        "Rec": {
            "recognition": "Custom",
            "custom_recognition": "MyRec",
            "action": "Custom",
            "custom_action": "MyAct",
            "custom_action_param": "👋哈哈哈(*´▽｀)ノノ😀",
        },
    }

    detail = tasker.post_pipeline("Entry", ppover).wait().get()
    if detail:
        print(f"pipeline detail: {detail}")
    else:
        print("pipeline failed")
        raise RuntimeError("pipeline failed")

    tasker.resource.post_path("C:/_maafw_testing_/aaabbbccc")
    tasker.clear_cache()
    inited = tasker.inited
    running = tasker.running

    Tasker.set_save_draw(True)
    Tasker.set_recording(True)
    Tasker.set_stdout_level(LoggingLevelEnum.All)
    Tasker.set_show_hit_draw(True)
    Tasker.set_log_dir(".")

    devices = Toolkit.find_adb_devices()
    print(f"devices: {devices}")
    desktop = Toolkit.find_desktop_windows()
    print(f"desktop: {desktop}")
    Toolkit.register_pi_custom_action("MyAct", MyAction())
    Toolkit.register_pi_custom_recognition("MyRec", MyRecognizer())
    # Toolkit.run_pi_cli("C:/_maafw_testing_/aaabbbccc", ".", True)

    global analyzed, runned
    if not analyzed or not runned:
        print("failed to run custom recognizer or action")
        raise RuntimeError("failed to run custom recognizer or action")


if __name__ == "__main__":
    main()
