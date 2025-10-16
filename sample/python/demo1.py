# python -m pip install maafw
from maa.event_sink import EventSink
from maa.tasker import Tasker
from maa.toolkit import Toolkit
from maa.context import Context, ContextEventSink
from maa.resource import Resource
from maa.controller import AdbController
from maa.custom_action import CustomAction
from maa.custom_recognition import CustomRecognition


# for register decorator
resource = Resource()


def main():
    user_path = "./"
    resource_path = "sample/resource"

    Toolkit.init_option(user_path)

    res_job = resource.post_bundle(resource_path)
    res_job.wait()

    # If not found on Windows, try running as administrator
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
    controller.post_connection().wait()

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

        # context is a reference, will override the pipeline for whole task
        context.override_pipeline({"MyCustomOCR": {"roi": [1, 1, 114, 514]}})
        # context.run_recognition ...

        # make a new context to override the pipeline, only for itself
        new_context = context.clone()
        new_context.override_pipeline({"MyCustomOCR": {"roi": [100, 200, 300, 400]}})
        reco_detail = new_context.run_recognition("MyCustomOCR", argv.image)

        click_job = context.tasker.controller.post_click(10, 20)
        click_job.wait()

        context.override_next(argv.node_name, ["TaskA", "TaskB"])

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
