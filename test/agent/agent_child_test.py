"""
AgentServer 端测试

测试范围:
1. AgentServer: 注册自定义识别器/动作、事件监听器、生命周期管理
2. Context: run_task/run_recognition/run_action、override_*、clone、anchor、hit_count
3. Tasker: get_*_detail、running/stopping/post_stop、clear_cache
4. Resource: get_node_data、node_list、custom_*_list、hash、override_*
5. Controller: 各种输入操作、post_key_down/up、post_scroll
"""

import os
from pathlib import Path
import sys
import io
import numpy

# Fix encoding issues on Windows (cp1252 cannot encode some Unicode characters)
if sys.stdout.encoding != "utf-8":
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding="utf-8", errors="replace")
if sys.stderr.encoding != "utf-8":
    sys.stderr = io.TextIOWrapper(sys.stderr.buffer, encoding="utf-8", errors="replace")

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
from maa.pipeline import JRecognitionType, JActionType, JOCR, JClick


analyzed: bool = False
runned: bool = False


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
            f"on MyRecognition.analyze, context: {context}, image: {argv.image.shape}, "
            f"task_detail: {argv.task_detail}, reco_name: {argv.custom_recognition_name}, "
            f"reco_param: {argv.custom_recognition_param}"
        )

        # ============================================================
        # Context API 测试
        # ============================================================
        entry = "ColorMatch"
        ppover = {
            "ColorMatch": {
                "recognition": "ColorMatch",
                "lower": [100, 100, 100],
                "upper": [255, 255, 255],
                "action": "Click",
            }
        }

        # 测试 run_task
        context.run_task(entry, ppover)

        # 测试 run_action
        action_detail = context.run_action(
            entry, [114, 514, 191, 810], "RunAction Detail", ppover
        )
        print(f"  action_detail: {action_detail}")

        # 测试 run_recognition
        reco_detail = context.run_recognition(entry, argv.image, ppover)
        print(f"  reco_detail: {reco_detail}")

        # 测试 run_recognition_direct
        reco_direct_detail = context.run_recognition_direct(
            JRecognitionType.OCR, JOCR(), argv.image
        )
        print(f"  reco_direct_detail: {reco_direct_detail}")

        # 测试 run_action_direct
        action_direct_detail = context.run_action_direct(
            JActionType.Click, JClick(), (100, 100, 50, 50), ""
        )
        print(f"  action_direct_detail: {action_direct_detail}")

        # 测试 clone 和 override
        new_ctx = context.clone()
        new_ctx.override_pipeline({"TaskA": {}, "TaskB": {}})
        new_ctx.override_next(argv.node_name, ["TaskA", "TaskB"])

        # 测试 get_node_data (Context 级别)
        node_data = new_ctx.get_node_data(argv.node_name)
        print(
            f"  ctx.get_node_data keys: {list(node_data.keys()) if node_data else None}"
        )

        # 测试 anchor API
        new_ctx.set_anchor("test_anchor", "TaskA")
        anchor_result = new_ctx.get_anchor("test_anchor")
        print(f"  anchor_result: {anchor_result}")
        assert (
            anchor_result == "TaskA"
        ), f"anchor should be 'TaskA', got {anchor_result}"

        # 测试 hit count API
        hit_count = new_ctx.get_hit_count(argv.node_name)
        print(f"  hit_count: {hit_count}")
        new_ctx.clear_hit_count(argv.node_name)

        # 测试 override_image (Context 级别)
        test_image = numpy.zeros((100, 100, 3), dtype=numpy.uint8)
        new_ctx.override_image("test_image", test_image)

        # 测试 get_task_job
        task_job = new_ctx.get_task_job()
        print(f"  task_job: {task_job}")
        new_task_detail = task_job.get()
        print(
            f"  task_detail entry: {new_task_detail.entry if new_task_detail else None}"
        )

        # ============================================================
        # Tasker API 测试 (通过 context.tasker)
        # ============================================================
        tasker = new_ctx.tasker
        print(f"  tasker.inited: {tasker.inited}")

        # 测试 get_latest_node
        node_detail = tasker.get_latest_node("ColorMatch")
        print(f"  latest_node ColorMatch: {node_detail}")

        # 测试 running 和 stopping
        print(f"  tasker.running: {tasker.running}")
        print(f"  tasker.stopping: {tasker.stopping}")

        # ============================================================
        # Resource API 测试 (通过 context.tasker.resource)
        # ============================================================
        resource = tasker.resource

        # 测试 loaded (valid) 属性
        print(f"  resource.loaded: {resource.loaded}")

        # 测试 get_node_data (Resource 级别)
        res_node_data = resource.get_node_data(argv.node_name)
        print(
            f"  res.get_node_data keys: {list(res_node_data.keys()) if res_node_data else None}"
        )

        # 测试 hash 属性
        res_hash = resource.hash
        print(f"  resource.hash: {res_hash[:16] if res_hash else None}...")

        # 测试 node_list
        node_list = resource.node_list
        print(f"  resource.node_list count: {len(node_list)}")

        # 测试 custom_recognition_list 和 custom_action_list
        reco_list = resource.custom_recognition_list
        action_list = resource.custom_action_list
        print(f"  custom_recognition_list: {reco_list}")
        print(f"  custom_action_list: {action_list}")
        assert "MyRec" in reco_list, "MyRec should be in custom_recognition_list"
        assert "MyAct" in action_list, "MyAct should be in custom_action_list"

        # 测试 get_default_recognition_param
        ocr_default = resource.get_default_recognition_param(JRecognitionType.OCR)
        print(f"  ocr_default: {ocr_default}")
        assert (
            ocr_default is not None
        ), "get_default_recognition_param should return value"

        # 测试 get_default_action_param
        click_default = resource.get_default_action_param(JActionType.Click)
        print(f"  click_default: {click_default}")
        assert click_default is not None, "get_default_action_param should return value"

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
            f"on MyAction.run, context: {context}, task_detail: {argv.task_detail}, "
            f"action_name: {argv.custom_action_name}, action_param: {argv.custom_action_param}, "
            f"box: {argv.box}, reco_detail: {argv.reco_detail}"
        )

        # ============================================================
        # Controller API 测试 (通过 context.tasker.controller)
        # ============================================================
        controller = context.tasker.controller

        # 测试 connected 和 uuid
        connected = controller.connected
        uuid = controller.uuid
        print(f"  connected: {connected}, uuid: {uuid}")

        # 测试 post_screencap
        new_image = controller.post_screencap().wait().get()
        print(f"  new_image: {new_image.shape}")

        # 测试 cached_image
        cached_image = controller.cached_image
        print(f"  cached_image shape: {cached_image.shape}")

        # 测试 resolution (需要在首次截图后才能获取有效值)
        resolution = controller.resolution
        print(f"  resolution: {resolution}")
        assert isinstance(resolution, tuple), "resolution should be a tuple"
        assert len(resolution) == 2, "resolution should have 2 elements"

        # 测试基本输入操作
        controller.post_click(191, 98).wait()
        controller.post_swipe(100, 200, 300, 400, 100).wait()
        controller.post_input_text("Hello World!").wait()
        controller.post_click_key(32).wait()

        # 测试触摸操作
        controller.post_touch_down(1, 100, 100, 0).wait()
        controller.post_touch_move(1, 200, 200, 0).wait()
        controller.post_touch_up(1).wait()

        # 测试按键操作
        controller.post_key_down(65).wait()
        controller.post_key_up(65).wait()

        # 测试滚动操作
        controller.post_scroll(0, 120).wait()

        # 测试应用操作
        controller.post_start_app("aaa")
        controller.post_stop_app("bbb")

        # ============================================================
        # Tasker API 补充测试 (详情获取)
        # ============================================================
        tasker = context.tasker

        # 获取当前任务详情用于后续测试
        task_job = context.get_task_job()
        task_detail = task_job.get()

        if task_detail:
            print(
                f"  task_detail: entry={task_detail.entry}, status={task_detail.status}"
            )

            # 测试 get_task_detail
            fetched_task_detail = tasker.get_task_detail(task_detail.task_id)
            print(
                f"  get_task_detail: {fetched_task_detail.entry if fetched_task_detail else None}"
            )

            # 测试 get_node_detail
            if task_detail.nodes:
                node = task_detail.nodes[0]
                node_detail = tasker.get_node_detail(node.node_id)
                print(f"  get_node_detail: {node_detail.name if node_detail else None}")

                # 测试 get_recognition_detail
                if node.recognition:
                    reco_detail = tasker.get_recognition_detail(
                        node.recognition.reco_id
                    )
                    print(
                        f"  get_recognition_detail: {reco_detail.name if reco_detail else None}"
                    )

        # 测试 clear_cache
        tasker.clear_cache()
        print("  tasker.clear_cache() called")

        global runned
        runned = True

        return CustomAction.RunResult(success=True)


# ============================================================================
# Event Sink 装饰器方式注册
# ============================================================================


@AgentServer.resource_sink()
class MyResSink(ResourceEventSink):
    def on_raw_notification(self, resource, msg: str, details: dict):
        print(f"[ResourceSink] msg: {msg}")


@AgentServer.controller_sink()
class MyCtrlSink(ControllerEventSink):
    def on_raw_notification(self, controller, msg: str, details: dict):
        print(f"[ControllerSink] msg: {msg}")


@AgentServer.tasker_sink()
class MyTaskerSink(TaskerEventSink):
    def on_raw_notification(self, tasker, msg: str, details: dict):
        print(f"[TaskerSink] msg: {msg}")


@AgentServer.context_sink()
class MyCtxSink(ContextEventSink):
    def on_raw_notification(self, context, msg: str, details: dict):
        print(f"[ContextSink] msg: {msg}")


if __name__ == "__main__":
    print(f"AgentServer MaaFw Version: {Library.version()}")

    Toolkit.init_option(install_dir / "bin")

    main()
