from typing import Tuple

# python -m pip install maafw
from maa.define import RectType
from maa.resource import Resource
from maa.controller import AdbController
from maa.tasker import Tasker
from maa.toolkit import Toolkit

from maa.custom_recognizer import CustomRecognizer
from maa.custom_action import CustomAction


def main():
    user_path = "./"
    Toolkit.init_option(user_path)

    resource = Resource()
    res_job = resource.post_path("sample/resource")
    res_job.wait()

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
    ctrl_job = controller.post_connection()
    ctrl_job.wait()

    tasker = Tasker()
    tasker.bind(resource, controller)

    if not tasker.inited:
        print("Failed to init MAA.")
        exit()

    my_rec = MyRecognizer()
    resource.register_custom_recognizer("MyRec", my_rec)

    task_detail = tasker.post_pipeline("StartUpAndClickButton").wait().get()
    # do something with task_detail

    reco_detail = tasker.post_recognition("MySingleMatch").wait().get()
    # do something with reco_detail


class MyRecognizer(CustomRecognizer):

    def analyze(
        self,
        context,
        task_detail,
        recognizer_name,
        custom_recognition_param,
        image,
    ) -> Tuple[bool, RectType, str]:
        reco_detail = context.run_recognition(
            "MyCustomOCR", image, pipeline_override={"roi": [100, 100, 200, 300]}
        )

        # context is a reference, will override the pipeline for whole task
        context.override_pipeline({"MyCustomOCR": {"roi": [1, 1, 114, 514]}})
        # context.run_recognition ...

        # make a new context to override the pipeline, only for itself
        new_context = context.clone()
        new_context.override_pipeline({"MyCustomOCR": {"roi": [100, 200, 300, 400]}})
        reco_detail = new_context.run_recognition("MyCustomOCR", image)

        click_job = context.tasker().controller().post_click(10, 20)
        click_job.wait()

        return True, (0, 0, 100, 100), "Hello World!"


if __name__ == "__main__":
    main()
