from pathlib import Path
from typing import Tuple
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
from maa.define import RectType


class MyRecognizer(CustomRecognizer):

    def analyze(
        self,
        context,
        task_detail,
        action_name,
        custom_param,
        image,
    ) -> Tuple[bool, RectType, str]:
        print(
            f"on MyRecognizer.analyze, image: {image.shape}, task_detail: {task_detail}, action_name: {action_name}, custom_param: {custom_param}"
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
        context.run_action(entry, ppover, [114, 514, 191, 810], "RunAction Detail")
        reco_detail = context.run_recognition(entry, image, ppover)
        print(f"reco_detail: {reco_detail}")
        return True, (11, 4, 5, 14), "Hello World!"


class MyAction(CustomAction):

    def run(
        self,
        context,
        task_detail,
        action_name,
        custom_param,
        box,
        reco_detail,
    ) -> bool:
        print(
            f"on MyAction.run, task_name: {task_detail}, action_name: {action_name}, custom_param: {custom_param}, box: {box}, reco_detail: {reco_detail}"
        )
        controller = context.tasker().controller()
        controller.screencap().wait()
        new_image = controller.cached_image()
        print(f"new_image: {new_image.shape}")
        controller.click(191, 98).wait()
        controller.swipe(100, 200, 300, 400, 100).wait()
        controller.input_text("Hello World!").wait()
        controller.press_key(32).wait()
        controller.touch_down(1, 100, 100, 0).wait()
        controller.touch_move(1, 200, 200, 0).wait()
        controller.touch_up(1).wait()
        return True


def main():
    version = Library.open(install_dir / "bin")
    print(f"MaaFw Version: {version}")

    Toolkit.init_option(install_dir / "bin")

    resource = Resource()
    print(f"resource: {hex(resource._handle)}")

    dbg_controller = DbgController(
        install_dir / "test" / "PipelineSmoking" / "Screenshot"
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
            "custom_param": "👋哈哈哈(*´▽｀)ノノ😀",
        },
    }

    detail = tasker.post_pipeline("Entry", ppover).get()
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
