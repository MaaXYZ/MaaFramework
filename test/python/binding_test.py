"""
Python binding API 测试

测试范围：
1. Resource: 资源加载、推理设置、自定义识别/动作注册、事件监听
2. Controller: 控制器连接、各种输入操作、截图、事件监听
3. Tasker: 任务执行、状态查询、详情获取、全局选项、事件监听
4. Context: 上下文操作、锚点、命中计数
5. Toolkit: 设备发现
6. CustomController: 自定义控制器

注意：Pipeline 解析相关的详细测试在 pipeline_test.py 中
"""

import os
from pathlib import Path
import sys
import numpy
import io

# Fix encoding issues on Windows
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
from maa.resource import Resource, ResourceEventSink
from maa.controller import DbgController, CustomController, ControllerEventSink
from maa.tasker import Tasker, TaskerEventSink
from maa.toolkit import Toolkit
from maa.custom_action import CustomAction
from maa.custom_recognition import CustomRecognition
from maa.define import MaaDbgControllerTypeEnum, LoggingLevelEnum
from maa.context import Context, ContextEventSink
from maa.event_sink import EventSink
from maa.pipeline import JRecognitionType, JActionType, JOCR, JClick

analyzed: bool = False
runned: bool = False


# ============================================================================
# Event Sink 实现
# ============================================================================


class MyResourceEventSink(ResourceEventSink):
    def _on_raw_notification(self, handle, msg: str, details: dict):
        print(f"  [ResourceSink] msg: {msg}")


class MyControllerEventSink(ControllerEventSink):
    def _on_raw_notification(self, handle, msg: str, details: dict):
        print(f"  [ControllerSink] msg: {msg}")


class MyTaskerEventSink(TaskerEventSink):
    def _on_raw_notification(self, handle, msg: str, details: dict):
        print(f"  [TaskerSink] msg: {msg}")


class MyContextEventSink(ContextEventSink):
    def _on_raw_notification(self, handle, msg: str, details: dict):
        print(f"  [ContextSink] msg: {msg}")


class MyEventSink(EventSink):
    def _on_raw_notification(self, handle, msg: str, details: dict):
        print(f"  [EventSink] msg: {msg}")


# ============================================================================
# 自定义识别和动作
# ============================================================================


class MyRecognition(CustomRecognition):

    def analyze(
        self,
        context: Context,
        argv: CustomRecognition.AnalyzeArg,
    ) -> CustomRecognition.AnalyzeResult:
        print(
            f"on MyRecognition.analyze, context: {context}, image: {argv.image.shape}"
        )

        # 测试 Context API
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
        print(f"  action_detail: {action_detail}")
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

        # 测试 get_node_data/get_node_object
        node_data = new_ctx.get_node_data(argv.node_name)
        node_obj = new_ctx.get_node_object(argv.node_name)
        print(f"  node_data keys: {list(node_data.keys()) if node_data else None}")

        # 测试 anchor API
        new_ctx.set_anchor("test_anchor", "TaskA")
        anchor_result = new_ctx.get_anchor("test_anchor")
        print(f"  anchor_result: {anchor_result}")

        # 测试 hit count API
        hit_count = new_ctx.get_hit_count(argv.node_name)
        print(f"  hit_count: {hit_count}")
        new_ctx.clear_hit_count(argv.node_name)

        # 测试 override_image
        test_image = numpy.zeros((100, 100, 3), dtype=numpy.uint8)
        new_ctx.override_image("test_image", test_image)

        # 测试从 resource/tasker 获取数据
        res_node_data = new_ctx.tasker.resource.get_node_data(argv.node_name)
        print(
            f"  res_node_data keys: {list(res_node_data.keys()) if res_node_data else None}"
        )

        node_detail = new_ctx.tasker.get_latest_node("ColorMatch")
        print(f"  node_detail: {node_detail}")

        task_job = new_ctx.get_task_job()
        new_task_detail = task_job.get()
        print(
            f"  task_detail entry: {new_task_detail.entry if new_task_detail else None}"
        )

        global analyzed
        analyzed = True

        return CustomRecognition.AnalyzeResult(
            box=(11, 4, 5, 14), detail={"message": "Hello World!"}
        )


class MyAction(CustomAction):
    def run(
        self,
        context: Context,
        argv: CustomAction.RunArg,
    ) -> CustomAction.RunResult:
        print(f"on MyAction.run, context: {context}, box: {argv.box}")
        controller = context.tasker.controller
        new_image = controller.post_screencap().wait().get()
        print(f"  new_image: {new_image.shape}")
        controller.post_click(191, 98).wait()
        controller.post_swipe(100, 200, 300, 400, 100).wait()
        controller.post_input_text("Hello World!").wait()
        controller.post_click_key(32).wait()
        controller.post_touch_down(1, 100, 100, 0).wait()
        controller.post_touch_move(1, 200, 200, 0).wait()
        controller.post_touch_up(1).wait()
        controller.post_key_down(65).wait()
        controller.post_key_up(65).wait()
        controller.post_scroll(0, 120).wait()
        controller.post_start_app("aaa")
        controller.post_stop_app("bbb")

        cached_image = controller.cached_image
        connected = controller.connected
        uuid = controller.uuid
        resolution = controller.resolution
        print(f"  connected: {connected}, uuid: {uuid}, resolution: {resolution}")

        global runned
        runned = True

        return CustomAction.RunResult(success=True)


# ============================================================================
# Resource API 测试
# ============================================================================


def test_resource_api():
    print("\n=== test_resource_api ===")

    # 测试推理设置 API
    r1 = Resource()
    r1.use_directml()
    r1.use_coreml()
    r1.use_auto_ep()
    r1.use_cpu()

    r2 = Resource()
    r2.use_directml(0)
    r2.use_cpu()

    # 测试无效路径加载（应该失败但不崩溃）
    r2.post_bundle("C:/_maafw_testing_/aaabbbccc").wait()

    # 测试 loaded 属性
    print(f"  r2.loaded (after invalid path): {r2.loaded}")

    # 测试事件监听器
    resource = Resource()
    sink = MyResourceEventSink()
    sink_id = resource.add_sink(sink)
    print(f"  sink_id: {sink_id}")

    # 加载有效资源
    resource.post_bundle(install_dir / "test" / "PipelineSmoking" / "resource").wait()
    print(f"  resource.loaded: {resource.loaded}")

    # 测试 hash 属性
    res_hash = resource.hash
    print(f"  resource.hash: {res_hash[:16]}...")

    # 测试自定义识别/动作注册
    my_reco = MyRecognition()
    my_action = MyAction()
    resource.register_custom_recognition("MyRec", my_reco)
    resource.register_custom_action("MyAct", my_action)

    # 测试 custom_recognition_list 和 custom_action_list
    reco_list = resource.custom_recognition_list
    action_list = resource.custom_action_list
    print(f"  custom_recognition_list: {reco_list}")
    print(f"  custom_action_list: {action_list}")
    assert "MyRec" in reco_list, "MyRec should be registered"
    assert "MyAct" in action_list, "MyAct should be registered"

    # 测试 node_list
    node_list = resource.node_list
    print(f"  node_list count: {len(node_list)}")

    # 测试 unregister
    resource.unregister_custom_recognition("MyRec")
    resource.unregister_custom_action("MyAct")
    reco_list_after = resource.custom_recognition_list
    action_list_after = resource.custom_action_list
    assert "MyRec" not in reco_list_after, "MyRec should be unregistered"
    assert "MyAct" not in action_list_after, "MyAct should be unregistered"

    # 重新注册用于后续测试
    resource.register_custom_recognition("MyRec", my_reco)
    resource.register_custom_action("MyAct", my_action)

    # 测试 override_pipeline (resource 级别)
    # 先创建被引用的节点
    resource.override_pipeline(
        {"SomeNode": {}, "TestOverride": {"action": "DoNothing"}}
    )
    override_data = resource.get_node_data("TestOverride")
    print(f"  override_data: {override_data}")

    # 测试 override_next (resource 级别)
    resource.override_next("TestOverride", ["SomeNode"])

    # 测试 override_image (resource 级别)
    test_img = numpy.zeros((100, 100, 3), dtype=numpy.uint8)
    resource.override_image("test_template", test_img)

    # 测试 get_default_recognition_param
    ocr_default = resource.get_default_recognition_param(JRecognitionType.OCR)
    print(f"  ocr_default: {ocr_default}")
    assert ocr_default is not None, "get_default_recognition_param should return value"

    template_default = resource.get_default_recognition_param(
        JRecognitionType.TemplateMatch
    )
    print(f"  template_default: {template_default}")
    assert (
        template_default is not None
    ), "get_default_recognition_param should return value"

    # 测试 get_default_action_param
    click_default = resource.get_default_action_param(JActionType.Click)
    print(f"  click_default: {click_default}")
    assert click_default is not None, "get_default_action_param should return value"

    swipe_default = resource.get_default_action_param(JActionType.Swipe)
    print(f"  swipe_default: {swipe_default}")
    assert swipe_default is not None, "get_default_action_param should return value"

    # 测试 remove_sink
    assert sink_id is not None, "sink_id should not be None"
    resource.remove_sink(sink_id)

    # 测试 clear_custom_recognition 和 clear_custom_action
    resource.clear_custom_recognition()
    resource.clear_custom_action()
    assert len(resource.custom_recognition_list) == 0, "should be empty after clear"
    assert len(resource.custom_action_list) == 0, "should be empty after clear"

    # 重新注册用于后续测试
    resource.register_custom_recognition("MyRec", my_reco)
    resource.register_custom_action("MyAct", my_action)

    print("  PASS: resource API")
    return resource


# ============================================================================
# Controller API 测试
# ============================================================================


def test_controller_api():
    print("\n=== test_controller_api ===")

    dbg_controller = DbgController(
        install_dir / "test" / "PipelineSmoking" / "Screenshot",
        install_dir / "test" / "user",
        MaaDbgControllerTypeEnum.CarouselImage,
    )
    print(f"  controller: {dbg_controller}")

    # 测试事件监听器
    sink = MyControllerEventSink()
    sink_id = dbg_controller.add_sink(sink)
    print(f"  sink_id: {sink_id}")

    # 连接
    dbg_controller.post_connection().wait()
    print(f"  connected: {dbg_controller.connected}")
    print(f"  uuid: {dbg_controller.uuid}")

    # 测试截图
    screencap_job = dbg_controller.post_screencap().wait()
    assert screencap_job.succeeded, "screencap should succeed"
    image = screencap_job.get()
    print(f"  screencap shape: {image.shape}")

    # 测试 cached_image
    cached = dbg_controller.cached_image
    print(f"  cached_image shape: {cached.shape}")

    # 测试 resolution (需要在首次截图后才能获取有效值)
    resolution = dbg_controller.resolution
    print(f"  resolution: {resolution}")
    assert isinstance(resolution, tuple), "resolution should be a tuple"
    assert len(resolution) == 2, "resolution should have 2 elements"
    assert isinstance(resolution[0], int), "resolution width should be int"
    assert isinstance(resolution[1], int), "resolution height should be int"

    # 测试输入操作
    dbg_controller.post_click(100, 100).wait()
    dbg_controller.post_swipe(100, 100, 200, 200, 100).wait()
    dbg_controller.post_click_key(32).wait()
    dbg_controller.post_key_down(65).wait()
    dbg_controller.post_key_up(65).wait()
    dbg_controller.post_input_text("test").wait()
    dbg_controller.post_touch_down(0, 100, 100, 0).wait()
    dbg_controller.post_touch_move(0, 150, 150, 0).wait()
    dbg_controller.post_touch_up(0).wait()
    dbg_controller.post_scroll(0, 120).wait()
    dbg_controller.post_start_app("com.test.app").wait()
    dbg_controller.post_stop_app("com.test.app").wait()

    # 测试截图选项
    dbg_controller.set_screenshot_target_long_side(1920)
    dbg_controller.set_screenshot_target_short_side(1080)
    dbg_controller.set_screenshot_use_raw_size(False)

    # 测试 remove_sink 和 clear_sinks
    assert sink_id is not None, "sink_id should not be None"
    dbg_controller.remove_sink(sink_id)
    dbg_controller.add_sink(MyControllerEventSink())
    dbg_controller.clear_sinks()

    print("  PASS: controller API")
    return dbg_controller


# ============================================================================
# Tasker API 测试
# ============================================================================


def test_tasker_api(resource: Resource, controller: DbgController):
    print("\n=== test_tasker_api ===")

    # 测试全局选项 (静态方法)
    Tasker.set_save_draw(True)
    Tasker.set_stdout_level(LoggingLevelEnum.All)
    Tasker.set_log_dir(".")
    Tasker.set_debug_mode(True)
    Tasker.set_save_on_error(True)
    Tasker.set_draw_quality(85)
    Tasker.set_reco_image_cache_limit(4096)

    # 创建 Tasker
    tasker = Tasker()
    print(f"  tasker: {tasker}")

    # 测试事件监听器
    tasker_sink = MyTaskerEventSink()
    context_sink = MyContextEventSink()
    tasker_sink_id = tasker.add_sink(tasker_sink)
    context_sink_id = tasker.add_context_sink(context_sink)
    print(f"  tasker_sink_id: {tasker_sink_id}, context_sink_id: {context_sink_id}")

    # 绑定资源和控制器
    tasker.bind(resource, controller)
    print(f"  inited: {tasker.inited}")

    if not tasker.inited:
        print("Failed to init tasker")
        raise RuntimeError("Failed to init tasker")

    # 测试 resource 和 controller 属性
    bound_resource = tasker.resource
    bound_controller = tasker.controller
    print(f"  bound_resource loaded: {bound_resource.loaded}")
    print(f"  bound_controller connected: {bound_controller.connected}")

    # 测试 post_task
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
        print(f"  task detail entry: {detail.entry}, status: {detail.status}")
        print(f"  task nodes count: {len(detail.nodes)}")

        # 测试 get_task_detail
        task_detail = tasker.get_task_detail(detail.task_id)
        print(f"  get_task_detail: {task_detail.entry if task_detail else None}")

        # 测试 get_node_detail
        if detail.nodes:
            node = detail.nodes[0]
            node_detail = tasker.get_node_detail(node.node_id)
            print(f"  get_node_detail: {node_detail.name if node_detail else None}")

            # 测试 get_recognition_detail
            if node.recognition:
                reco_detail = tasker.get_recognition_detail(node.recognition.reco_id)
                print(
                    f"  get_recognition_detail: {reco_detail.name if reco_detail else None}"
                )

            # 测试 get_action_detail
            if node.action:
                action_detail = tasker.get_action_detail(node.action.action_id)
                print(
                    f"  get_action_detail: {action_detail.name if action_detail else None}"
                )
    else:
        print("Pipeline task failed")
        raise RuntimeError("Pipeline task failed")

    # 测试 running 和 stopping
    print(f"  running: {tasker.running}")
    print(f"  stopping: {tasker.stopping}")

    # 测试 post_stop
    tasker.post_task("Entry", ppover)
    stop_job = tasker.post_stop()
    print(f"  stopping after post_stop: {tasker.stopping}")
    stop_job.wait()
    print(f"  stopping after wait: {tasker.stopping}")

    # 测试 get_latest_node
    latest_node = tasker.get_latest_node("Rec")
    print(f"  latest_node: {latest_node.name if latest_node else None}")

    # 测试 clear_cache
    tasker.clear_cache()

    # 测试 override_pipeline (通过 job 对象)
    task_job = tasker.post_task("Entry", ppover)
    override_result = task_job.override_pipeline({"Entry": {"next": []}})
    print(f"  task_job.override_pipeline result: {override_result}")
    task_job.wait()

    # 测试 remove_sink 和 clear_sinks
    assert tasker_sink_id is not None, "tasker_sink_id should not be None"
    assert context_sink_id is not None, "context_sink_id should not be None"
    tasker.remove_sink(tasker_sink_id)
    tasker.remove_context_sink(context_sink_id)
    tasker.add_sink(MyTaskerEventSink())
    tasker.add_context_sink(MyContextEventSink())
    tasker.clear_sinks()
    tasker.clear_context_sinks()

    print("  PASS: tasker API")
    return tasker


# ============================================================================
# CustomController 测试
# ============================================================================


class MyController(CustomController):

    def __init__(self):
        super().__init__()
        self.count = 0

    def connect(self) -> bool:
        print("  on MyController.connect")
        self.count += 1
        return True

    def connected(self) -> bool:
        print("on MyController.connected")
        return True

    def request_uuid(self) -> str:
        print("  on MyController.request_uuid")
        return "12345678"

    def start_app(self, intent: str) -> bool:
        print(f"  on MyController.start_app: {intent}")
        self.count += 1
        return True

    def stop_app(self, intent: str) -> bool:
        print(f"  on MyController.stop_app: {intent}")
        self.count += 1
        return True

    def screencap(self) -> numpy.ndarray:
        print("  on MyController.screencap")
        self.count += 1
        return numpy.zeros((1080, 1920, 3), dtype=numpy.uint8)

    def click(self, x: int, y: int) -> bool:
        print(f"  on MyController.click: {x}, {y}")
        self.count += 1
        return True

    def swipe(self, x1: int, y1: int, x2: int, y2: int, duration: int) -> bool:
        print(f"  on MyController.swipe: {x1}, {y1} -> {x2}, {y2}, {duration}")
        self.count += 1
        return True

    def touch_down(self, contact: int, x: int, y: int, pressure: int) -> bool:
        print(f"  on MyController.touch_down: {contact}, {x}, {y}")
        self.count += 1
        return True

    def touch_move(self, contact: int, x: int, y: int, pressure: int) -> bool:
        print(f"  on MyController.touch_move: {contact}, {x}, {y}")
        self.count += 1
        return True

    def touch_up(self, contact: int) -> bool:
        print(f"  on MyController.touch_up: {contact}")
        self.count += 1
        return True

    def click_key(self, keycode: int) -> bool:
        print(f"  on MyController.click_key: {keycode}")
        self.count += 1
        return True

    def input_text(self, text: str) -> bool:
        print(f"  on MyController.input_text: {text}")
        self.count += 1
        return True

    def key_down(self, keycode: int) -> bool:
        print(f"  on MyController.key_down: {keycode}")
        self.count += 1
        return True

    def key_up(self, keycode: int) -> bool:
        print(f"  on MyController.key_up: {keycode}")
        self.count += 1
        return True

    def scroll(self, dx: int, dy: int) -> bool:
        print(f"  on MyController.scroll: {dx}, {dy}")
        self.count += 1
        return True


def test_custom_controller():
    print("\n=== test_custom_controller ===")

    controller = MyController()
    controller.add_sink(MyControllerEventSink())

    ret = controller.post_connection().wait().succeeded
    uuid = controller.uuid
    print(f"  uuid: {uuid}")

    ret &= controller.post_start_app("custom_aaa").wait().succeeded
    ret &= controller.post_stop_app("custom_bbb").wait().succeeded

    image_job = controller.post_screencap().wait()
    ret &= image_job.succeeded
    print(f"  image shape: {image_job.get().shape}")

    ret &= controller.post_click(100, 200).wait().succeeded
    ret &= controller.post_swipe(100, 200, 300, 400, 200).wait().succeeded
    ret &= controller.post_touch_down(1, 100, 100, 0).wait().succeeded
    ret &= controller.post_touch_move(1, 200, 200, 0).wait().succeeded
    ret &= controller.post_touch_up(1).wait().succeeded
    ret &= controller.post_click_key(32).wait().succeeded
    ret &= controller.post_input_text("Hello World!").wait().succeeded
    ret &= controller.post_key_down(65).wait().succeeded
    ret &= controller.post_key_up(65).wait().succeeded
    ret &= controller.post_scroll(0, 120).wait().succeeded

    print(f"  controller.count: {controller.count}, ret: {ret}")
    print("  PASS: custom controller")


# ============================================================================
# Toolkit 测试
# ============================================================================


def test_toolkit():
    print("\n=== test_toolkit ===")

    devices = Toolkit.find_adb_devices()
    print(f"  adb devices: {len(devices)}")
    for dev in devices[:3]:
        print(f"    - {dev.name}: {dev.address}")

    desktop = Toolkit.find_desktop_windows()
    print(f"  desktop windows: {len(desktop)}")
    for win in desktop[:3]:
        print(f"    - {win.window_name[:30] if win.window_name else '(no name)'}")

    print("  PASS: toolkit")


# ============================================================================
# 主入口
# ============================================================================


if __name__ == "__main__":
    print(f"MaaFw Version: {Library.version()}")

    Toolkit.init_option(install_dir / "bin")

    # 测试各模块 API
    resource = test_resource_api()
    controller = test_controller_api()
    tasker = test_tasker_api(resource, controller)

    # 验证自定义识别和动作被调用
    if not analyzed or not runned:
        print("FAIL: custom recognition or action not called")
        raise RuntimeError("custom recognition or action not called")

    # 测试 CustomController
    test_custom_controller()

    # 测试 Toolkit
    test_toolkit()

    print("\n" + "=" * 50)
    print("All binding tests passed!")
    print("=" * 50)
