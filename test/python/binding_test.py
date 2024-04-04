from pathlib import Path
from typing import Tuple
import sys
import asyncio

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
from maa.instance import Instance
from maa.toolkit import Toolkit
from maa.custom_action import CustomAction
from maa.custom_recognizer import CustomRecognizer
from maa.define import RectType


class MyRecognizer(CustomRecognizer):
    def analyze(
        self, context, image, task_name, custom_param
    ) -> Tuple[bool, RectType, str]:
        print(
            f"on MyRecognizer.analyze, image: {image.shape}, task_name: {task_name}, custom_param: {custom_param}"
        )
        entry = "ColorMatch"
        param = {
            "ColorMatch": {
                "recognition": "ColorMatch",
                "lower": [100, 100, 100],
                "upper": [255, 255, 255],
                "action": "Click",
            }
        }
        context.run_task(entry, param)
        context.run_action(entry, param, [114, 514, 191, 810], "RunAction Detail")
        rec_res = context.run_recognizer(image, entry, param)
        print(f"rec_res: {rec_res}")
        return True, (11, 4, 5, 14), "Hello World!"


class MyAction(CustomAction):
    def run(self, context, task_name, custom_param, box, rec_detail) -> bool:
        print(
            f"on MyAction.run, task_name: {task_name}, custom_param: {custom_param}, box: {box}, rec_detail: {rec_detail}"
        )
        new_image = context.screencap()
        print(f"new_image: {new_image.shape}")
        context.click(191, 98)
        context.swipe(100, 200, 300, 400, 100)
        context.input_text("Hello World!")
        context.press_key(32)
        context.touch_down(1, 100, 100, 0)
        context.touch_move(1, 200, 200, 0)
        context.touch_up(1)
        return True

    def stop(self) -> None:
        pass


my_rec = MyRecognizer()
my_act = MyAction()


async def main():
    version = Library.open(install_dir / "bin")
    print(f"MaaFw Version: {version}")

    Toolkit.init_option(install_dir / "bin")

    resource = Resource()
    print(f"resource: {hex(resource._handle)}")

    dbg_controller = DbgController(
        install_dir / "test" / "PipelineSmoking" / "Screenshot"
    )
    print(f"controller: {hex(dbg_controller._handle)}")
    await dbg_controller.connect()

    maa_inst = Instance()
    maa_inst.bind(resource, dbg_controller)
    print(f"maa_inst: {hex(maa_inst._handle)}")

    if not maa_inst.inited:
        print("failed to init maa_inst")
        exit(1)

    maa_inst.register_action("MyAct", my_act)
    maa_inst.register_recognizer("MyRec", my_rec)

    await maa_inst.run_task(
        "Entry",
        {
            "Entry": {"next": "Rec"},
            "Rec": {
                "recognition": "Custom",
                "custom_recognition": "MyRec",
                "action": "Custom",
                "custom_action": "MyAct",
                "custom_action_param": "abcdefg",
            },
        },
    )

    print("Done.")


if __name__ == "__main__":
    asyncio.run(main())
