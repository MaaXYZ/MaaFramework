"""
AgentClient 端测试

测试范围:
1. AgentClient: 创建、绑定、连接管理、超时设置
2. AgentClient: custom_recognition_list、custom_action_list
3. 与 AgentServer 的完整交互流程
"""

import os
from pathlib import Path
import sys

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
from maa.controller import DbgController
from maa.tasker import Tasker
from maa.agent_client import AgentClient
from maa.toolkit import Toolkit
from maa.custom_action import CustomAction
from agent_test_utils import (
    CONNECT_TIMEOUT_MS,
    create_marker_path,
    run_connected_agent_test,
    start_agent_server,
    stop_agent_server,
)


class ConflictingAction(CustomAction):
    def run(self, context, argv):
        return True


def api_test():
    # ============================================================
    # 创建并初始化 Resource, Controller, Tasker
    # ============================================================
    resource = Resource()
    print(f"resource: {resource}")

    # 加载资源
    resource.post_bundle(
        install_dir / "test" / "PipelineSmoking" / "resource"
    ).wait()
    print(f"resource.loaded: {resource.loaded}")

    dbg_controller = DbgController(
        install_dir / "test" / "PipelineSmoking" / "Screenshot",
    )
    print(f"controller: {dbg_controller}")
    dbg_controller.post_connection().wait()

    tasker = Tasker()
    tasker.bind(resource, dbg_controller)
    print(f"tasker: {tasker}")

    if not tasker.inited:
        print("failed to init tasker")
        exit(1)

    # ============================================================
    # AgentClient API 测试
    # ============================================================
    agent = AgentClient()
    print(f"agent: {agent}")

    # 测试 identifier
    socket_id = agent.identifier
    print(f"agent.identifier: {socket_id}")
    if not socket_id:
        print("failed to get identifier")
        exit(1)

    # 测试 bind
    if not agent.bind(resource):
        print("failed to bind resource")
        exit(1)
    print("agent.bind(resource) succeeded")

    # 测试 register_sink
    if not agent.register_sink(resource, dbg_controller, tasker):
        print("failed to register sink")
        exit(1)
    print("agent.register_sink() succeeded")

    # ============================================================
    # 超时测试
    # ============================================================
    if not agent.set_timeout(0):
        print("failed to set timeout to 0")
        exit(1)
    print("agent.set_timeout(0) succeeded")

    # 尝试连接（应该超时失败，因为还没有 AgentServer）
    if agent.connect():
        print("unexpected: connect should timeout")
        exit(1)
    print("agent.connect() timed out as expected")

    # 恢复正常超时
    if not agent.set_timeout(-1):
        print("failed to set timeout to -1")
        exit(1)
    print("agent.set_timeout(-1) succeeded")

    # ============================================================
    # 启动 AgentServer 子进程
    # ============================================================
    agent_server_command = [
        sys.executable,
        str(Path(__file__).parent / "agent_child_test.py"),
        str(binding_dir),
        str(install_dir),
        socket_id,
    ]
    child_process = start_agent_server(agent_server_command)

    try:
        assert agent.set_timeout(CONNECT_TIMEOUT_MS)
        conflicting_action = ConflictingAction()
        assert resource.register_custom_action("MyAct", conflicting_action)
        assert not agent.connect(), "connect should fail on duplicate custom name"
        assert not agent.connected, "agent should remain disconnected after registration rollback"
        assert "MyAct" in resource.custom_action_list, "existing custom action should be preserved"
        assert "MyRec" not in resource.custom_recognition_list, "agent registrations should be rolled back"
        child_process.wait(timeout=10)
        assert child_process.returncode == 0, "failed connect should stop the server automatically"
    finally:
        assert agent.set_timeout(-1)
        stop_agent_server(child_process)

    assert resource.unregister_custom_action("MyAct")

    sink_report_file = create_marker_path("agent-sinks")
    run_connected_agent_test(
        agent,
        agent_server_command,
        sink_report_file,
        resource,
        tasker,
        install_dir / "test" / "PipelineSmoking" / "resource" / "pipeline",
    )

    # 验证断开连接后的状态
    print(f"agent.connected after disconnect: {agent.connected}")

    print("\n" + "=" * 50)
    print("All agent tests passed!")
    print("=" * 50)


if __name__ == "__main__":
    print(f"AgentClient MaaFw Version: {Library.version()}")

    Toolkit.init_option(install_dir / "bin")

    api_test()
