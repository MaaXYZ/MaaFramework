import os
from pathlib import Path
import sys
import subprocess

if len(sys.argv) < 3:
    print("Usage: python agent_main_test.py <binding_dir> <install_dir>")
    sys.exit(1)

binding_dir = Path(sys.argv[1]).resolve()
install_dir = Path(sys.argv[2]).resolve()

os.environ["MAAFW_BINARY_PATH"] = str(f"{install_dir}/bin")
print(f"binding_dir: {binding_dir}")
print(f"install_dir: {install_dir}")

if str(binding_dir) not in sys.path:
    sys.path.insert(0, str(binding_dir))

from maa.library import Library
from maa.resource import Resource
from maa.controller import DbgController, CustomController, MaaDbgControllerTypeEnum
from maa.tasker import Tasker
from maa.agent_client import AgentClient
from maa.toolkit import Toolkit


def api_test():
    resource = Resource()
    print(f"resource: {resource}")

    dbg_controller = DbgController(
        install_dir / "test" / "PipelineSmoking" / "Screenshot",
        install_dir / "test" / "user",
        MaaDbgControllerTypeEnum.CarouselImage,
    )
    print(f"controller: {dbg_controller}")
    dbg_controller.post_connection().wait()

    tasker = Tasker()
    tasker.bind(resource, dbg_controller)
    print(f"tasker: {tasker}")

    if not tasker.inited:
        print("failed to init tasker")
        exit(1)

    agent = AgentClient()
    agent.bind(resource)
    agent.register_sink(resource, dbg_controller, tasker)
    socket_id = agent.identifier

    if not socket_id:
        print("failed to create socket")
        exit(1)
        
    if not agent.set_timeout(0):
        print("failed to set timeout")
        exit(1)
    if agent.connect():
        print("failed to connect timeout")
        exit(1)
    if not agent.set_timeout(-1):
        print("failed to set timeout")
        exit(1)

    subprocess.Popen(
        [
            "python",
            str(Path(__file__).parent / "agent_child_test.py"),
            str(binding_dir),
            str(install_dir),
            socket_id,
        ],
    )

    if not agent.connect():
        print("failed to connect")
        exit(1)
    if not agent.connected:
        print("agent is not connected")
        exit(1)
    if not agent.alive:
        print("agent is not alive")
        exit(1)

    ppover = {
        "Entry": {"next": "Rec"},
        "Rec": {
            "recognition": "Custom",
            "custom_recognition": "MyRec",
            "action": "Custom",
            "custom_action": "MyAct",
            "custom_action_param": "哈哈哈(*´▽｀)ノノ",
        },
    }
    detail = tasker.post_task("Entry", ppover).wait().get()
    if detail:
        print(f"pipeline detail: {detail}")
    else:
        print("pipeline failed")
        raise RuntimeError("pipeline failed")

    agent.disconnect()

if __name__ == "__main__":
    print(f"AgentClient MaaFw Version: {Library.version()}")

    Toolkit.init_option(install_dir / "bin")

    api_test()
