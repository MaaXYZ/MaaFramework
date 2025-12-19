# python -m pip install maafw
from maa.event_sink import EventSink
from maa.tasker import Tasker
from maa.toolkit import Toolkit
from maa.context import Context, ContextEventSink
from maa.resource import Resource
from maa.controller import (
    AdbController,
    Win32Controller,
    MaaWin32ScreencapMethodEnum,
    MaaWin32InputMethodEnum,
)
from maa.custom_action import CustomAction
from maa.custom_recognition import CustomRecognition


# for register decorator
resource = Resource()


def main():
    user_path = "./"
    Toolkit.init_option(user_path)

    ### For ADB controller ###

    # If not found, try running as administrator
    adb_devices = Toolkit.find_adb_devices()
    if not adb_devices:
        print("No ADB device found.")
        exit()

    # for demo, we just use the first device
    device = adb_devices[0]
    controller = AdbController(
        adb_path=device.adb_path,
        address=device.address,
        screencap_methods=device.screencap_methods,
        input_methods=device.input_methods,
        config=device.config,
    )

    ### for Win32 controller ###

    all_windows = Toolkit.find_desktop_windows()
    if not all_windows:
        print("No window found.")
        exit()
    # if can not control the window, try running as administrator
    # or use other screencap/input method
    controller = Win32Controller(
        hwnd=all_windows[0].hwnd,  # for demo, we just use the first Window
        screencap_method=MaaWin32ScreencapMethodEnum.FramePool,
        mouse_method=MaaWin32InputMethodEnum.PostMessage,
        keyboard_method=MaaWin32InputMethodEnum.PostMessage,
    )

    controller.post_connection().wait()

    # screenshot
    image = controller.post_screencap().wait().get()
    # do something with image
    # ...

    # sync click with waiting
    controller.post_click(100, 200).wait()
    # async click without waiting
    click_job = controller.post_click(100, 200)
    # do something else
    # ...
    click_job.wait()

    controller.post_input_text("Hello MAA!").wait()

    # Load resource for do recognition and action

    resource_path = "sample/resource"
    res_job = resource.post_bundle(resource_path)
    res_job.wait()

    tasker = Tasker()
    tasker.bind(resource, controller)

    if not tasker.inited:
        print("Failed to init MAA.")
        exit()

    # just an example, use it in json
    pipeline_override = {
        "MyCustomEntry": {"action": "custom", "custom_action": "MyCustomAction"},
    }

    # another way to register
    # resource.register_custom_recognition("My_Recongition", MyRecongition())
    # resource.register_custom_action("My_CustomAction", MyCustomAction())

    task_detail = tasker.post_task("MyCustomEntry", pipeline_override).wait().get()
    # do something with task_detail

    tasker.controller.post_screencap().wait()


# auto register by decorator, can also call `resource.register_custom_recognition` manually
@resource.custom_recognition("MyRecongition")
class MyRecongition(CustomRecognition):

    def analyze(
        self,
        context,
        argv: CustomRecognition.AnalyzeArg,
    ) -> CustomRecognition.AnalyzeResult:
        reco_detail = context.run_recognition(
            "MyCustomOCR",
            argv.image,
            pipeline_override={
                "MyCustomOCR": {"recognition": "OCR", "roi": [100, 100, 200, 300]}
            },
        )

        new_image = context.tasker.controller.post_screencap().wait().get()

        # async click without waiting
        click_job = context.tasker.controller.post_click(10, 20)

        # context is a reference, will override the pipeline for whole task
        context.override_pipeline({"MyCustomOCR": {"roi": [1, 1, 114, 514]}})
        # context.run_recognition ...

        # check stopping after your atomic operation, and return immediately
        if context.tasker.stopping:
            return CustomRecognition.AnalyzeResult(
                box=(0, 0, 0, 0), detail="Task Stopped"
            )

        # wait the click done
        click_job.wait()

        # make a new context to override the pipeline, only for itself
        new_context = context.clone()
        new_context.override_pipeline({"MyCustomOCR": {"roi": [100, 200, 300, 400]}})
        reco_detail = new_context.run_recognition("MyCustomOCR", argv.image)
        if reco_detail and reco_detail.hit:
            # get result you want from reco_detail
            box = reco_detail.best_result.box
            context.tasker.controller.post_click(box[0], box[1]).wait()

        return CustomRecognition.AnalyzeResult(
            box=(0, 0, 100, 100), detail="Hello World!"
        )


class MyContextEventSink(ContextEventSink):
    def on_node_next_list(
        self,
        context: "Context",
        noti_type: EventSink.NotificationType,
        detail: ContextEventSink.NodeNextListDetail,
    ):
        pass

    def on_node_recognition(
        self,
        context: "Context",
        noti_type: EventSink.NotificationType,
        detail: ContextEventSink.NodeRecognitionDetail,
    ):
        pass

    def on_node_action(
        self,
        context: "Context",
        noti_type: EventSink.NotificationType,
        detail: ContextEventSink.NodeActionDetail,
    ):
        pass


# auto register by decorator, can also call `resource.register_custom_action` manually
@resource.custom_action("MyCustomAction")
class MyCustomAction(CustomAction):

    def run(
        self,
        context: Context,
        argv: CustomAction.RunArg,
    ) -> bool:
        """
        :param argv:
        :param context: 运行上下文
        :return: 是否执行成功。-参考流水线协议 `on_error`
        """
        print("MyCustomAction is running!")
        return True


if __name__ == "__main__":
    main()
