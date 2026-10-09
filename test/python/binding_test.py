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
import subprocess
import sys
import textwrap
import ctypes
import numpy
import io
from typing import Optional

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
from maa.controller import DbgController, CustomController, Win32Controller, KWinController, ControllerEventSink
from maa.tasker import Tasker, TaskerEventSink
from maa.toolkit import Toolkit
from maa.custom_action import CustomAction
from maa.custom_recognition import CustomRecognition
from maa.buffer import ImageBuffer
from maa.define import (
    LoggingLevelEnum,
    MaaWin32InputMethodEnum,
    MaaBool,
    MaaControllerHandle,
    MaaCtrlId,
    MaaStringBufferHandle,
)
from maa.context import Context, ContextEventSink
from maa.event_sink import EventSink
from maa.pipeline import JRecognitionType, JActionType, JOCR, JClick, JTemplateMatch

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

        # 失败动作也应保留 action_id 和动作类型，便于关联失败事件
        failed_action_detail = context.run_action_direct(
            JActionType.Click,
            JClick(target="__missing_target__"),
            (100, 100, 50, 50),
            "",
        )
        assert failed_action_detail is not None
        assert failed_action_detail.action_id != 0
        assert failed_action_detail.action == JActionType.Click
        assert not failed_action_detail.success

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

        # 测试 wait_freezes API（参数校验：time 和 wait_freezes_param.time 同时为零应返回 false）
        from maa.pipeline import JWaitFreezes

        wait_cases = [
            ("both zero", None),
            ("both zero, with box", (10, 10, 100, 100)),
        ]
        for case_name, box in wait_cases:
            wait_result = new_ctx.wait_freezes(
                time=0,
                box=box,
                wait_freezes_param=JWaitFreezes(time=0),
            )
            print(f"  wait_freezes ({case_name}): {wait_result}")
            assert (
                not wait_result
            ), "wait_freezes should return false when both time are zero"

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
        controller.post_start_app("aaa")
        controller.post_stop_app("bbb")
        controller.post_inactive().wait()

        cached_image = controller.cached_image
        connected = controller.connected
        uuid = controller.uuid
        resolution = controller.resolution
        info = controller.info
        print(
            f"  connected: {connected}, uuid: {uuid}, resolution: {resolution}, info type: {info.get('type')}"
        )

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
    r1.use_webgpu()
    r1.use_coreml()
    r1.use_auto_ep()
    r1.use_cpu()

    r2 = Resource()
    r2.use_directml(0)
    r2.use_webgpu(0)
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
    assert resource.register_custom_recognition("MyRec", my_reco)
    assert resource.register_custom_action("MyAct", my_action)

    duplicate_reco = MyRecognition()
    duplicate_action = MyAction()
    assert not resource.register_custom_recognition("MyRec", duplicate_reco)
    assert not resource.register_custom_action("MyAct", duplicate_action)
    assert not resource.register_custom_action("MyRec", duplicate_action)
    assert not resource.register_custom_recognition("MyAct", duplicate_reco)
    assert resource._custom_recognition_holder["MyRec"] is my_reco
    assert resource._custom_action_holder["MyAct"] is my_action
    assert resource.register_custom_recognition("CaseSensitive", MyRecognition())
    assert resource.register_custom_action("casesensitive", MyAction())
    assert not resource.register_custom_recognition("", MyRecognition())
    assert not resource.register_custom_action("", MyAction())

    try:
        resource.custom_recognition("MyAct")(MyRecognition)
        assert False, "duplicate custom decorator should raise RuntimeError"
    except RuntimeError as error:
        assert str(error) == "Custom name is already registered: 'MyAct'"

    try:
        resource.custom_action("MyRec")(MyAction)
        assert False, "duplicate custom decorator should raise RuntimeError"
    except RuntimeError as error:
        assert str(error) == "Custom name is already registered: 'MyRec'"

    for custom_decorator in [resource.custom_recognition, resource.custom_action]:
        try:
            custom_decorator("")
            assert False, "empty custom name should raise ValueError"
        except ValueError as error:
            assert str(error) == "Custom name must not be empty"

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
    assert resource.register_custom_recognition("MyRec", my_reco)
    assert resource.register_custom_action("MyAct", my_action)

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

    # 测试 info
    info = dbg_controller.info
    print(f"  info: {info}")
    assert isinstance(info, dict), "info should be a dict"
    assert "type" in info, "info should contain 'type'"
    assert info["type"] == "dbg", "dbg controller type should be 'dbg'"

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
    assert not dbg_controller.post_scroll(0, 120).wait().succeeded, (
        "dbg controller scroll should fail"
    )
    dbg_controller.post_start_app("com.test.app").wait()
    dbg_controller.post_stop_app("com.test.app").wait()
    dbg_controller.post_inactive().wait()

    # 测试截图选项
    dbg_controller.set_screenshot_target_long_side(1920)
    dbg_controller.set_screenshot_target_short_side(1080)
    dbg_controller.set_screenshot_use_raw_size(False)
    dbg_controller.set_screenshot_resize_method(3)  # INTER_AREA

    # 测试 remove_sink 和 clear_sinks
    assert sink_id is not None, "sink_id should not be None"
    dbg_controller.remove_sink(sink_id)
    dbg_controller.add_sink(MyControllerEventSink())
    dbg_controller.clear_sinks()

    print("  PASS: controller API")
    return dbg_controller


# ============================================================================
# Buffer API 测试
# ============================================================================


def test_buffer_api():
    print("\n=== test_buffer_api ===")

    buf = ImageBuffer()
    src = numpy.zeros((100, 200, 3), dtype=numpy.uint8)
    assert buf.set(src), "set should succeed"

    # 仅指定宽度，按比例缩放高度
    assert buf.resize(50, 0), "resize should succeed"
    resized = buf.get()
    print(f"  resized shape: {resized.shape}")
    assert resized.shape[1] == 50, "width should be 50"
    assert resized.shape[0] == 25, "height should keep aspect ratio"

    # 四通道 BGRA 按 BGR 存储，alpha 丢弃
    bgra = numpy.zeros((8, 6, 4), dtype=numpy.uint8)
    bgra[:, :, 0] = 11  # B
    bgra[:, :, 1] = 22  # G
    bgra[:, :, 2] = 33  # R
    bgra[:, :, 3] = 255  # A
    assert buf.set(bgra), "set BGRA should succeed"
    bgr = buf.get()
    print(f"  BGRA stored as: {bgr.shape}")
    assert bgr.shape == (8, 6, 3), f"BGRA should be stored as BGR, got {bgr.shape}"
    assert (bgr[:, :, 0] == 11).all(), "B channel should be preserved"
    assert (bgr[:, :, 1] == 22).all(), "G channel should be preserved"
    assert (bgr[:, :, 2] == 33).all(), "R channel should be preserved"

    # 单通道输入：二维与 (h, w, 1) 都按灰度复制到三通道
    for gray in (numpy.full((4, 6), 77, dtype=numpy.uint8), numpy.full((4, 6, 1), 77, dtype=numpy.uint8)):
        assert buf.set(gray), "set gray should succeed"
        gray_out = buf.get()
        print(f"  gray stored as: {gray_out.shape}")
        assert gray_out.shape == (4, 6, 3), f"gray should be replicated to BGR, got {gray_out.shape}"
        assert (gray_out == 77).all(), "gray value should be preserved in all channels"

    # 非连续切片
    padded = numpy.zeros((10, 10, 3), dtype=numpy.uint8)
    padded[:, :, 0] = 9
    view = padded[2:8, 3:9]
    assert not view.flags["C_CONTIGUOUS"], "view should be non-contiguous"
    assert buf.set(view), "set non-contiguous should succeed"
    view_out = buf.get()
    print(f"  non-contiguous stored as: {view_out.shape}")
    assert view_out.shape == (6, 6, 3), f"non-contiguous shape should be kept, got {view_out.shape}"
    assert (view_out[:, :, 0] == 9).all(), "non-contiguous content should be kept"

    # 非 uint8 数据无法按 uint8 解析，显式失败
    try:
        buf.set(numpy.zeros((4, 4, 3), dtype=numpy.float32))
    except TypeError:
        pass
    else:
        raise AssertionError("set with float32 should raise TypeError")

    # 通道数不受支持
    try:
        buf.set(numpy.zeros((4, 4, 2), dtype=numpy.uint8))
    except ValueError:
        pass
    else:
        raise AssertionError("set with 2 channels should raise ValueError")

    print("  PASS: buffer API")


# ============================================================================
# Tasker API 测试
# ============================================================================


def test_tasker_api(resource: Resource, controller: DbgController):
    print("\n=== test_tasker_api ===")

    # 测试全局选项 (静态方法)
    Tasker.set_save_draw(True)
    Tasker.set_stdout_level(LoggingLevelEnum.All)
    log_dir = install_dir / "bin" / "debug" / "新建文件夹"
    assert Tasker.set_log_dir(log_dir)
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

    def __init__(self, image: Optional[numpy.ndarray] = None):
        super().__init__()
        self.count = 0
        self.image = image if image is not None else numpy.zeros((1080, 1920, 3), dtype=numpy.uint8)

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
        return self.image

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

    def shell(self, cmd: str, timeout: int) -> Optional[str]:
        print(f"  on MyController.shell: {cmd}, {timeout}")
        self.count += 1
        return f"shell_output:{cmd}"

    def get_custom_info(self) -> dict:
        return {
            "custom_key": "custom_value",
            "answer": 42,
        }


def test_custom_controller():
    print("\n=== test_custom_controller ===")

    controller = MyController()
    controller.add_sink(MyControllerEventSink())

    ret = controller.post_connection().wait().succeeded
    uuid = controller.uuid
    print(f"  uuid: {uuid}")
    info = controller.info
    print(f"  info: {info}")
    assert isinstance(info, dict), "info should be a dict"
    assert info.get("type") == "custom", "info type should be custom"
    assert (
        info.get("custom_key") == "custom_value"
    ), "custom info should contain custom_key"
    assert info.get("answer") == 42, "custom info should contain answer"

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
    ret &= controller.post_inactive().wait().succeeded

    shell_job = controller.post_shell("echo hello", 5000).wait()
    assert shell_job.done, "post_shell job must complete"
    print(f"  post_shell status: {shell_job.status}, output: {controller.shell_output!r}")

    print(f"  controller.count: {controller.count}, ret: {ret}")

    # 非法的截图数据（此处为非 uint8）必须是受控的截图失败，而不是被静默误读成错图
    bad_controller = MyController(numpy.zeros((8, 6, 3), dtype=numpy.float32))
    assert bad_controller.post_connection().wait().succeeded, "bad controller connect should succeed"
    print("  以下 traceback 是预期的：非法截图数据被拒绝")
    assert not bad_controller.post_screencap().wait().succeeded, "invalid screencap data should fail the job"

    print("  PASS: custom controller")


def _new_command_image_tasker() -> Tasker:
    resource = Resource()
    resource.post_bundle(install_dir / "test" / "PipelineSmoking" / "resource").wait()
    controller = MyController()
    assert controller.post_connection().wait().succeeded
    tasker = Tasker()
    tasker.bind(resource, controller)
    assert tasker.inited
    return tasker


def _run_command_image(tasker: Tasker, marker: Path):
    marker.unlink(missing_ok=True)
    script = "import os,sys; open(sys.argv[2],'w').write(str(os.path.isfile(sys.argv[1])))"
    detail = (
        tasker.post_task(
            "CommandImage",
            {
                "CommandImage": {
                    "action": "Command",
                    "exec": sys.executable,
                    "args": ["-c", script, "{IMAGE}", str(marker)],
                }
            },
        )
        .wait()
        .get()
    )
    assert detail and detail.nodes, "CommandImage task should have node detail"
    return detail.nodes[0].action.success


def test_command_image_placeholder():
    """回归：{IMAGE} 在第二个 Tasker 上崩溃（static 表绑定悬空 this）；无缓存截图时 OpenCV 断言终止进程"""
    print("\n=== test_command_image_placeholder ===")

    import tempfile

    marker = Path(tempfile.gettempdir()) / "maafw_command_image_marker.txt"

    # 无缓存截图：应按需补截图，而不是让 imwrite 断言终止进程
    no_shot = _new_command_image_tasker()
    assert _run_command_image(no_shot, marker), "no cached image should screencap on demand"
    assert marker.read_text() == "True", "image file should exist after on-demand screencap"

    # 同一进程内多个 Tasker（旧的保持存活）都应能使用 {IMAGE}
    alive = []
    for i in range(3):
        tasker = _new_command_image_tasker()
        alive.append(tasker)
        assert tasker.controller.post_screencap().wait().succeeded
        assert _run_command_image(tasker, marker), f"tasker #{i} Command {{IMAGE}} failed"
        assert marker.read_text() == "True", f"tasker #{i} image file should exist"

    marker.unlink(missing_ok=True)
    print("  PASS: Command {IMAGE} placeholder")


def _noise_pattern(seed: int, size: int) -> numpy.ndarray:
    blocks = numpy.random.default_rng(seed).integers(0, 256, size=(size // 4, size // 4, 3), dtype=numpy.uint8)
    return blocks.repeat(4, axis=0).repeat(4, axis=1)


def _write_png(path: Path, image: numpy.ndarray):
    """测试环境没有 OpenCV/PIL，手写一个最简 PNG 编码"""
    import struct
    import zlib

    def chunk(tag: bytes, data: bytes) -> bytes:
        return struct.pack(">I", len(data)) + tag + data + struct.pack(">I", zlib.crc32(tag + data))

    height, width, _ = image.shape
    rgb = image[:, :, ::-1]
    raw = b"".join(b"\x00" + rgb[y].tobytes() for y in range(height))
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(
        b"\x89PNG\r\n\x1a\n"
        + chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0))
        + chunk(b"IDAT", zlib.compress(raw))
        + chunk(b"IEND", b"")
    )


def _template_sizes(bundles: list[Path], template: str, screen: numpy.ndarray) -> set:
    resource = Resource()
    for bundle in bundles:
        assert resource.post_bundle(bundle).wait().succeeded, f"load bundle failed: {bundle}"
    controller = MyController(screen)
    assert controller.post_connection().wait().succeeded
    tasker = Tasker()
    tasker.bind(resource, controller)
    assert tasker.inited

    param = JTemplateMatch(template=[template], threshold=[0.9])
    detail = tasker.post_recognition(JRecognitionType.TemplateMatch, param, screen).wait().get()
    assert detail and detail.nodes, "recognition should have node detail"
    reco = detail.nodes[0].recognition
    # 框的宽高即模板宽高，用来区分参与匹配的是哪张图
    return {(r.box[2], r.box[3]) for r in reco.all_results}


def test_template_override_across_bundles():
    """回归 #1541：多个 Bundle 中同路径的模板图片应以后加载的为准，文件夹按相对路径合并"""
    print("\n=== test_template_override_across_bundles ===")

    import shutil
    import tempfile

    pattern_a = _noise_pattern(1, 40)
    pattern_b = _noise_pattern(2, 60)
    pattern_c = _noise_pattern(3, 48)
    screen = numpy.full((720, 1280, 3), 128, dtype=numpy.uint8)
    screen[100:140, 100:140] = pattern_a

    work = Path(tempfile.mkdtemp(prefix="maafw_template_override_"))
    try:
        base = work / "base"
        override = work / "override"
        _write_png(base / "image" / "T.png", pattern_a)
        _write_png(base / "image" / "D" / "1.png", pattern_a)
        _write_png(base / "image" / "D" / "2.png", pattern_c)
        _write_png(override / "image" / "T.png", pattern_b)
        _write_png(override / "image" / "D" / "1.png", pattern_b)

        sizes = _template_sizes([base], "T.png", screen)
        assert sizes == {(40, 40)}, f"base only: {sizes}"

        sizes = _template_sizes([base, override], "T.png", screen)
        assert sizes == {(60, 60)}, f"file should be replaced by the later bundle: {sizes}"

        sizes = _template_sizes([base, override], "D", screen)
        assert sizes == {(60, 60), (48, 48)}, f"directory should merge by relative path: {sizes}"
    finally:
        shutil.rmtree(work, ignore_errors=True)

    print("  PASS: template override across bundles")


def _ctypes_type_name(ctypes_type):
    """取 ctypes 类型名；restype 可能为 None（void），此时回退到 repr。"""
    return getattr(ctypes_type, "__name__", repr(ctypes_type))


def _assert_ctypes_signature(name, func, argtypes, restype):
    """断言 ctypes 函数签名逐项匹配，失败时给出具体位置。"""
    assert func.argtypes is not None, f"{name}.argtypes must be declared"
    assert len(func.argtypes) == len(argtypes), (
        f"{name}.argtypes: expected {len(argtypes)} args, got {len(func.argtypes)}"
    )
    for index, (got, want) in enumerate(zip(func.argtypes, argtypes)):
        assert got is want, (
            f"{name}.argtypes[{index}]: expected"
            f" {_ctypes_type_name(want)}, got {_ctypes_type_name(got)}"
        )
    assert func.restype is restype, (
        f"{name}.restype: expected"
        f" {_ctypes_type_name(restype)}, got {_ctypes_type_name(func.restype)}"
    )


def test_shell_api_declarations():
    """回归守卫：shell API 的 ctypes 签名必须与 C 头文件一致。

    仅断言「已声明」不足以拦截错误的宽度——把 64 位句柄或 timeout 误声明为
    c_int 同样会溢出，而 ctypes 在 argtypes 缺失时也正是按 32 位 int 编组。
    因此逐项断言完整签名。声明缺失导致的溢出/返回值截断取决于句柄/ID 数值，
    调用式测试无法确定性复现。
    """
    print("\n=== test_shell_api_declarations ===")

    framework = Library.framework()

    # MaaCtrlId MaaControllerPostShell(MaaController*, const char*, int64_t)
    _assert_ctypes_signature(
        "MaaControllerPostShell",
        framework.MaaControllerPostShell,
        [MaaControllerHandle, ctypes.c_char_p, ctypes.c_int64],
        MaaCtrlId,
    )

    # MaaBool MaaControllerGetShellOutput(const MaaController*, MaaStringBuffer*)
    _assert_ctypes_signature(
        "MaaControllerGetShellOutput",
        framework.MaaControllerGetShellOutput,
        [MaaControllerHandle, MaaStringBufferHandle],
        MaaBool,
    )

    print("  PASS: shell API ctypes signatures match the C header")


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

    instances = Toolkit.find_gamescope_instances()
    print(f"  gamescope instances: {len(instances)}")
    for inst in instances[:3]:
        print(f"    - display_no={inst.display_no} node_id={inst.pipewire_node_id} eis={inst.eis_socket_path}")

    print("  PASS: toolkit")


def test_background_managed_keys_api():
    print("\n=== test_background_managed_keys_api ===")

    # Test with DbgController (non-Win32, should fail)
    dbg_controller = DbgController(
        install_dir / "test" / "PipelineSmoking" / "Screenshot",
    )
    dbg_ret = dbg_controller.set_background_managed_keys([0x57, 0x41])
    print(f"  dbg_controller set_background_managed_keys: {dbg_ret}")
    assert not dbg_ret, "DbgController should not support BackgroundManagedKeys"

    # Test with Win32 controller if available
    desktop_windows = Toolkit.find_desktop_windows()
    if desktop_windows:
        win32_controller = None
        for window in desktop_windows:
            try:
                win32_controller = Win32Controller(window.hwnd)
                break
            except RuntimeError:
                continue

        if win32_controller is not None:
            # Set option before connection
            ret = win32_controller.set_background_managed_keys([0x57, 0x41])
            print(
                f"  win32_controller set_background_managed_keys (before connection): {ret}"
            )
            assert (
                ret
            ), "Win32Controller should support BackgroundManagedKeys before connection"

            # After connection, setting non-empty array should succeed
            win32_controller.post_connection().wait()
            ret_post = win32_controller.set_background_managed_keys([0x57, 0x41])
            print(
                f"  win32_controller set_background_managed_keys (after connection): {ret_post}"
            )
            assert (
                ret_post
            ), "Win32Controller should support BackgroundManagedKeys after connection"

            # Empty array should clear managed keys
            ret_clear = win32_controller.set_background_managed_keys([])
            print(
                f"  win32_controller set_background_managed_keys (clear with empty): {ret_clear}"
            )
            assert (
                ret_clear
            ), "Win32Controller should support clearing BackgroundManagedKeys with empty array"
        else:
            print("  SKIP: failed to create Win32 controller")
    else:
        print("  SKIP: no desktop windows found for Win32 test")

    print("  PASS: background managed keys API")


def test_win32_relative_move():
    print("\n=== test_win32_relative_move ===")

    desktop_windows = Toolkit.find_desktop_windows()
    if not desktop_windows:
        print("  SKIP: no desktop windows found")
        return

    controller = None
    target_window = None
    for window in desktop_windows:
        try:
            controller = Win32Controller(window.hwnd)
            target_window = window
            break
        except RuntimeError:
            continue

    if controller is None or target_window is None:
        print("  SKIP: failed to create Win32 controller")
        return

    ret = controller.post_connection().wait().succeeded
    ret &= controller.post_relative_move(0, 0).wait().succeeded

    print(
        f"  target window: {target_window.window_name[:30] if target_window.window_name else '(no name)'}"
    )
    print(f"  ret: {ret}")
    assert ret, "win32 relative_move should succeed"
    print("  PASS: win32 relative_move")


def test_kwin_controller_create():
    print("\n=== test_kwin_controller_create ===")

    # KWinController 仅在 Linux 上可用，且需要 MaaKWinControllerCreate API 存在
    try:
        controller = KWinController(
            device_node="/dev/uinput",
            screen_width=1920,
            screen_height=1080,
            use_win32_vk_code=False,
        )
        print(f"  KWinController created: {controller}")

        # 检查连接前状态
        print(f"  connected: {controller.connected}")
        print(f"  uuid: {controller.uuid}")
        print(f"  info: {controller.info}")

        # 验证 info 中的类型
        info = controller.info
        assert isinstance(info, dict), "info should be a dict"
        assert "type" in info, "info should contain 'type'"
        assert info["type"] == "KWin", "KWin controller type should be 'KWin'"

        # 测试 post_inactive (空操作，应总是成功)
        controller.post_inactive().wait()

        print("  PASS: KWinController creation")

    except RuntimeError as e:
        # KWin 控制器创建可能因 API 缺失或缺少 /dev/uinput 权限等环境问题失败
        print(f"  SKIP: KWinController not available in this environment ({e})")


def test_win32_interception_enum():
    print("\n=== test_win32_interception_enum ===")
    assert int(MaaWin32InputMethodEnum.Interception) == 1 << 9
    print("  PASS: win32 interception enum")


def test_win32_anchored_touch_enum():
    print("\n=== test_win32_anchored_touch_enum ===")
    assert int(MaaWin32InputMethodEnum.AnchoredTouch) == 1 << 10
    print("  PASS: win32 anchored touch enum")


# ============================================================================
# 主入口
# ============================================================================


def test_binding_init_thread_safety():
    """并发初始化回归测试 / Concurrent initialisation regression test (#629)

    ctypes 的 argtypes/restype 是进程级一次性初始化，必须在子进程中验证：
    父进程早已完成初始化，竞态窗口不复存在。
    """
    print("\n=== test_binding_init_thread_safety ===")

    child = textwrap.dedent(
        """
        import ctypes, sys, threading
        from maa.controller import AdbController

        N = 8
        barrier, errors = threading.Barrier(N), []

        def worker(i):
            barrier.wait()          # 最大化重叠在动态库懒加载上
            try:
                ctrl = AdbController(adb_path="adb", address=f"127.0.0.1:{16384 + i * 32}")
                ctrl.post_connection().wait()
            except (ctypes.ArgumentError, OSError) as e:
                errors.append(f"{type(e).__name__}: {e}")
            except Exception:
                pass                # 连接失败是预期的，与本测试无关

        ts = [threading.Thread(target=worker, args=(i,)) for i in range(N)]
        for t in ts: t.start()
        for t in ts: t.join()

        if errors:
            print("RACE " + errors[0])
            sys.exit(1)
        sys.exit(0)
        """
    )

    env = dict(
        os.environ,
        MAAFW_BINARY_PATH=str(install_dir / "bin"),
        PYTHONPATH=str(binding_dir),
    )

    # 竞态是概率性的（单次命中率约 95%），重复几次把漏报压到千分之一以下
    for _ in range(3):
        proc = subprocess.run(
            [sys.executable, "-c", child],
            env=env,
            capture_output=True,
            text=True,
            timeout=180,
        )
        if proc.returncode == 0:
            continue
        race = [l for l in proc.stdout.splitlines() if l.startswith("RACE ")]
        detail = race[0][5:] if race else f"child exited {proc.returncode} (crashed?)"
        print(f"  FAIL: concurrent binding init is not thread-safe -- {detail}")
        raise RuntimeError(f"binding init race (#629): {detail}")

    print("  PASS: 8 threads initialised the binding concurrently")


if __name__ == "__main__":
    print(f"MaaFw Version: {Library.version()}")

    Toolkit.init_option(install_dir / "bin")

    # 测试各模块 API
    resource = test_resource_api()
    controller = test_controller_api()
    test_buffer_api()
    tasker = test_tasker_api(resource, controller)

    # 验证自定义识别和动作被调用
    if not analyzed or not runned:
        print("FAIL: custom recognition or action not called")
        raise RuntimeError("custom recognition or action not called")

    # 测试 CustomController
    test_custom_controller()

    # 回归：Command 动作 {IMAGE} 占位符
    test_command_image_placeholder()

    # 回归：多 Bundle 同名模板覆盖 (#1541)
    test_template_override_across_bundles()

    # shell API 的 ctypes 声明
    test_shell_api_declarations()

    # 测试 Toolkit
    test_toolkit()

    # 测试 BackgroundManagedKeys 选项
    test_background_managed_keys_api()

    # 测试 Win32 relative_move 正路径
    test_win32_relative_move()

    # 测试 KWinController 创建
    test_kwin_controller_create()

    # 测试 Win32 Interception 枚举导出
    test_win32_interception_enum()

    # 测试 Win32 AnchoredTouch 枚举导出
    test_win32_anchored_touch_enum()

    # 回归：并发初始化线程安全 (#629)
    test_binding_init_thread_safety()

    print("\n" + "=" * 50)
    print("All binding tests passed!")
    print("=" * 50)
