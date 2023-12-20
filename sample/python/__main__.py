from maa.library import Library
from maa.resource import Resource
from maa.controller import AdbController
from maa.instance import Instance
from maa.toolkit import Toolkit

from maa.custom_recognizer import CustomRecognizer
from maa.custom_action import CustomAction


class MyRecognizer(CustomRecognizer):
    def analyze(
        self, context, image, task_name, custom_recognition_param
    ) -> (bool, (int, int, int, int), str):
        return True, (0, 0, 100, 100), "Hello World!"


class MyAction(CustomAction):
    def run(self, context, task_name, custom_param, cur_box, cur_rec_detail) -> bool:
        return True

    def stop(self) -> None:
        pass


my_rec = MyRecognizer()
my_act = MyAction()

if __name__ == "__main__":
    version = Library.open("bin")
    print(f"MaaFw Version: {version}")

    Toolkit.init_config()

    resource = Resource()
    res_id = resource.post_path("sample/resource")
    resource.wait(res_id)

    device_list = Toolkit.adb_devices()
    if not device_list:
        print("No ADB device found.")
        exit()

    # for demo, we just use the first device
    device = device_list[0]
    controller = AdbController(
        adb_path=device.adb_path,
        address=device.address,
        agent_path="share/MaaAgentBinary",
    )
    ctrl_id = controller.post_connection()
    controller.wait(ctrl_id)

    maa_inst = Instance()
    maa_inst.bind(resource, controller)

    if not maa_inst.inited():
        print("Failed to init MAA.")
        exit()

    maa_inst.register_custom_recognizer("MyRec", my_rec)
    maa_inst.register_custom_action("MyAct", my_act)

    task_id = maa_inst.post_task("StartUpAndClickButton", {})
    maa_inst.wait(task_id)
