from pathlib import Path
from typing import Tuple, Optional
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
from maa.define import RectType, MaaDbgControllerTypeEnum
from maa.context import Context


class MyRecognizer(CustomRecognizer):

    def analyze(
        self,
        context: Context,
        argv: CustomRecognizer.AnalyzeArg,
    ) -> CustomRecognizer.AnalyzeResult:
        print(
            f"on MyRecognizer.analyze, context: {context}, image: {argv.image.shape}, task_detail: {argv.task_detail}, action_name: {action_name}, custom_action_param: {custom_action_param}"
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

        return CustomRecognizer.AnalyzeResult(box=(11, 4, 5, 14), detail="Hello World!")


class MyAction(CustomAction):

    def run(
        self,
        context: Context,
        task_detail,
        action_name,
        custom_action_param,
        box,
        reco_detail,
    ) -> bool:
        print(
            f"on MyAction.run, context: {context}, task_detail: {task_detail}, action_name: {action_name}, custom_action_param: {custom_action_param}, box: {box}, reco_detail: {reco_detail}"
        )
        controller = context.tasker().controller()
        controller.post_screencap().wait()
        new_image = controller.cached_image()
        print(f"new_image: {new_image.shape}")
        controller.post_click(191, 98).wait()
        controller.post_swipe(100, 200, 300, 400, 100).wait()
        controller.post_input_text("Hello World!").wait()
        controller.post_press_key(32).wait()
        controller.post_touch_down(1, 100, 100, 0).wait()
        controller.post_touch_move(1, 200, 200, 0).wait()
        controller.post_touch_up(1).wait()
        return True


def main():
    version = Library.open(install_dir / "bin")
    print(f"MaaFw Version: {version}")

    Toolkit.init_option(install_dir / "bin")

    resource = Resource()
    print(f"resource: {hex(resource._handle)}")

    dbg_controller = DbgController(
        install_dir / "test" / "PipelineSmoking" / "Screenshot",
        install_dir / "test" / "user",
        MaaDbgControllerTypeEnum.CarouselImage,
    )
    print(f"controller: {hex(dbg_controller._handle)}")
    dbg_controller.post_connection().wait()

    tasker = Tasker()
    tasker.bind(resource, dbg_controller)
    print(f"tasker: {hex(tasker._handle)}")

    if not tasker.inited:
        print("failed to init tasker")
        exit(1)

    my_rec = MyRecognizer()
    my_act = MyAction()
    resource.register_custom_action("MyAct", my_act)
    resource.register_custom_recognizer("MyRec", my_rec)

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

    detail = tasker.post_recognition("Rec", ppover).wait().get()
    if detail:
        print(f"reco detail: {detail}")
    else:
        print("reco failed")
        raise RuntimeError("reco failed")

    detail = tasker.post_action("Rec", ppover).wait().get()
    if detail:
        print(f"action detail: {detail}")
    else:
        print("action failed")
        raise RuntimeError("action failed")

    print("Done.")


if __name__ == "__main__":
    main()
