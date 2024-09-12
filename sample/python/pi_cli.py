from maa.toolkit import Toolkit
from maa.context import Context
from maa.custom_action import CustomAction
from maa.custom_recognizer import CustomRecognizer


def main():
    # 注册自定义动作
    Toolkit.register_pi_custom_action("MyAction", MyAction())

    # 启动 MaaPiCli
    Toolkit.run_pi_cli("C:/MaaXXX/resource", "C:/MaaXXX/cache", False)


class MyAction(CustomAction):
    def run(
        self, context: Context, argv: CustomAction.RunArg
    ) -> CustomAction.RunResult:

        print(f"on MyAction.run, context: {context}, argv: {argv}")

        context.override_next(argv.current_task_name, ["TaskA", "TaskB"])

        image = context.tasker.controller.cached_image
        context.tasker.controller.post_click(100, 100).wait()

        reco_detail = context.run_recognition(
            "Cat", image, {"Cat": {"recognition": "OCR", "expected": "喵喵喵"}}
        )
        # if reco_detail xxxx

        return CustomAction.RunResult(success=True)
