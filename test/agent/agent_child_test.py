import os
from pathlib import Path
import sys
import io

# Fix encoding issues on Windows (cp1252 cannot encode some Unicode characters)
if sys.stdout.encoding != 'utf-8':
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8', errors='replace')
if sys.stderr.encoding != 'utf-8':
    sys.stderr = io.TextIOWrapper(sys.stderr.buffer, encoding='utf-8', errors='replace')

if len(sys.argv) < 4:
    print("Call agent_main_test.py instead of this file.")
    sys.exit(1)

binding_dir = Path(sys.argv[1]).resolve()
install_dir = Path(sys.argv[2]).resolve()

os.environ["MAAFW_BINARY_PATH"] = str(f"{install_dir}/bin")
print(f"binding_dir: {binding_dir}")
print(f"install_dir: {install_dir}")

if str(binding_dir) not in sys.path:
    sys.path.insert(0, str(binding_dir))

# Must be imported first
from maa.agent.agent_server import AgentServer

from maa.resource import ResourceEventSink
from maa.controller import ControllerEventSink
from maa.tasker import TaskerEventSink
from maa.context import Context, ContextEventSink
from maa.custom_action import CustomAction
from maa.custom_recognition import CustomRecognition
from maa.toolkit import Toolkit
from maa.library import Library


def main():
    if len(sys.argv) < 2:
        print("Usage: python agent_main.py <socket_id>")
        exit(1)

    socket_id = sys.argv[-1]
    AgentServer.start_up(socket_id)
    AgentServer.join()
    AgentServer.shut_down()


@AgentServer.custom_recognition("MyRec")
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
        action_detail = context.run_action(entry, [114, 514, 191, 810], "RunAction Detail", ppover)
        print(f"action_detail: {action_detail}")
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


@AgentServer.custom_action("MyAct")
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

        global runned
        runned = True

        return CustomAction.RunResult(success=True)


@AgentServer.resource_sink()
class MyResSink(ResourceEventSink):
    def on_raw_notification(self, resource, msg: str, details: dict):
        print(f"resource: {resource}, msg: {msg}, details: {details} ")


@AgentServer.controller_sink()
class MyCtrlSink(ControllerEventSink):
    def on_raw_notification(self, controller, msg: str, details: dict):
        print(f"controller: {controller}, msg: {msg}, details: {details} ")


@AgentServer.tasker_sink()
class MyTaskerSink(TaskerEventSink):
    def on_raw_notification(self, tasker, msg: str, details: dict):
        print(f"tasker: {tasker}, msg: {msg}, details: {details} ")


@AgentServer.context_sink()
class MyCtxSink(ContextEventSink):
    def on_raw_notification(self, context, msg: str, details: dict):
        print(f"context: {context}, msg: {msg}, details: {details} ")


if __name__ == "__main__":
    print(f"AgentServer MaaFw Version: {Library.version()}")

    Toolkit.init_option(install_dir / "bin")

    main()
