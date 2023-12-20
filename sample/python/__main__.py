from maa.library import Library
from maa.resource import Resource
from maa.controller import AdbController
from maa.instance import Instance

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
    Library.open("bin")

    resource = Resource()
    res_id = resource.post_path("sample/resource")
    resource.wait(res_id)

    controller = AdbController(
        "adb", "127.0.0.1:16416", agent_path="share/MaaAgentBinary"
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
