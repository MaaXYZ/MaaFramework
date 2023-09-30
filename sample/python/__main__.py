from maa.library import Library
from maa.resource import Resource
from maa.controller import AdbController
from maa.instance import MaaInstance

if __name__ == '__main__':

    Library.open('../../x64/Debug')

    resource = Resource()
    res_id = resource.post_path('../../x64/Debug/resource')
    resource.wait(res_id)

    controller = AdbController('adb', '127.0.0.1:16416')
    ctrl_id = controller.post_connection()
    controller.wait(ctrl_id)

    maa_instance = MaaInstance()
    maa_instance.bind(resource, controller)

    if not maa_instance.inited():
        print('Failed to init MAA instance.')
        exit()

    task_id = maa_instance.post_task('MyTask', {})
    maa_instance.wait(task_id)
