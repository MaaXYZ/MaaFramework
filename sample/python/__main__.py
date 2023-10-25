from maa.library import Library
from maa.resource import Resource
from maa.controller import AdbController
from maa.maa import Maa


if __name__ == "__main__":
    Library.open("bin")

    resource = Resource()
    res_id = resource.post_path("my_resource")
    resource.wait(res_id)

    controller = AdbController("adb", "127.0.0.1:16416", agent_path = "share/MaaAgentBinary")
    ctrl_id = controller.post_connection()
    controller.wait(ctrl_id)

    maa_inst = Maa()
    maa_inst.bind(resource, controller)

    if not maa_inst.inited():
        print("Failed to init MAA.")
        exit()

    task_id = maa_inst.post_task("StartUpAndClickButton", {})
    maa_inst.wait(task_id)
