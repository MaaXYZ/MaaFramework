"""
AgentClient TCP 端测试

测试范围:
1. AgentClient TCP 模式: 创建、绑定、连接管理、超时设置
2. AgentClient: custom_recognition_list、custom_action_list
3. 与 AgentServer 的完整交互流程 (通过 TCP)
"""

import os
from pathlib import Path
import socket
import sys
import subprocess
import time

if len(sys.argv) < 3:
    print("Usage: python agent_tcp_main_test.py <binding_dir> <install_dir>")
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


class ConflictingAction(CustomAction):
    def run(self, context, argv):
        return True


def run_startup_failure_test(
    agent: AgentClient,
    socket_id: str,
    mode: str,
    timeout_ms: int,
):
    executable_name = "AgentFaultServer.exe" if sys.platform == "win32" else "AgentFaultServer"
    executable = install_dir / "bin" / executable_name
    child_process = subprocess.Popen(
        [str(executable), mode, socket_id],
        stdout=subprocess.PIPE,
        text=True,
    )

    try:
        assert child_process.stdout is not None
        ready_message = child_process.stdout.readline().strip()
        assert ready_message == "ready", f"{mode} server failed before readiness: {ready_message}"
        assert agent.set_timeout(timeout_ms)
        started_at = time.monotonic()
        assert not agent.connect(), f"connect should fail in {mode} mode"
        elapsed = time.monotonic() - started_at
        assert elapsed < 2, f"{mode} cleanup should be bounded, took {elapsed:.3f}s"
        child_process.wait(timeout=10)
        assert child_process.returncode == 0, f"{mode} server should receive automatic shutdown"
    finally:
        if child_process.poll() is None:
            child_process.terminate()
            child_process.wait(timeout=10)

    assert agent.set_timeout(-1)


NUMERIC_IDENTIFIER_FLAG = "--numeric-identifier-flow"


def prepare_runtime():
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

    return resource, dbg_controller, tasker


def reserve_tcp_port():
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.bind(("127.0.0.1", 0))
        return sock.getsockname()[1]


def run_tcp_flow(
    agent: AgentClient,
    socket_id: str,
    *,
    scenario: str,
    test_startup_failures: bool = False,
):
    resource, dbg_controller, tasker = prepare_runtime()

    print(f"agent ({scenario}): {agent}")

    # 测试 identifier（TCP 模式下应为端口号字符串）
    print(f"agent.identifier: {socket_id}")
    if not socket_id or not socket_id.isdigit():
        print(f"unexpected identifier: {socket_id}, expected a port number string")
        exit(1)
    print(f"TCP port: {socket_id}")

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

    if test_startup_failures:
        run_startup_failure_test(agent, socket_id, "protocol-mismatch", 5000)
        run_startup_failure_test(agent, socket_id, "drop-startup-response", 200)
        fault_conflict = ConflictingAction()
        assert resource.register_custom_action("FaultConflict", fault_conflict)
        try:
            run_startup_failure_test(agent, socket_id, "registration-conflict", 5000)
            assert "FaultConflict" in resource.custom_action_list
        finally:
            assert resource.unregister_custom_action("FaultConflict")

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
    # 启动 AgentServer 子进程 (TCP 模式)
    # ============================================================
    child_process = subprocess.Popen(
        [
            sys.executable,
            str(Path(__file__).parent / "agent_tcp_child_test.py"),
            str(binding_dir),
            str(install_dir),
            socket_id,  # 传递端口号字符串
        ],
    )

    try:
        conflicting_action = ConflictingAction()
        assert resource.register_custom_action("MyAct", conflicting_action)
        assert not agent.connect(), "connect should fail on duplicate custom name"
        assert not agent.connected, "agent should remain disconnected after registration rollback"
        assert "MyAct" in resource.custom_action_list, "existing custom action should be preserved"
        assert "MyRec" not in resource.custom_recognition_list, "agent registrations should be rolled back"
        child_process.wait(timeout=10)
        assert child_process.returncode == 0, "failed connect should stop the server automatically"
    finally:
        if child_process.poll() is None:
            agent.disconnect()
            child_process.terminate()
            child_process.wait(timeout=10)

    assert resource.unregister_custom_action("MyAct")

    child_process = subprocess.Popen(
        [
            sys.executable,
            str(Path(__file__).parent / "agent_tcp_child_test.py"),
            str(binding_dir),
            str(install_dir),
            socket_id,
        ],
    )

    # 等待连接
    if not agent.connect():
        print("failed to connect to agent server via TCP")
        exit(1)
    print("agent.connect() via TCP succeeded")

    # 测试 connected 和 alive
    if not agent.connected:
        print("agent is not connected")
        exit(1)
    print(f"agent.connected: {agent.connected}")

    if not agent.alive:
        print("agent is not alive")
        exit(1)
    print(f"agent.alive: {agent.alive}")

    # ============================================================
    # 测试 custom_recognition_list 和 custom_action_list
    # ============================================================
    reco_list = agent.custom_recognition_list
    action_list = agent.custom_action_list
    print(f"agent.custom_recognition_list: {reco_list}")
    print(f"agent.custom_action_list: {action_list}")

    assert "MyRec" in reco_list, f"MyRec should be in custom_recognition_list, got {reco_list}"
    assert "MyAct" in action_list, f"MyAct should be in custom_action_list, got {action_list}"

    # ============================================================
    # 执行 Pipeline 任务
    # ============================================================
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
        print(f"pipeline detail: entry={detail.entry}, status={detail.status}")
        print(f"pipeline nodes count: {len(detail.nodes)}")

        # 测试获取详细信息
        if detail.nodes:
            node = detail.nodes[0]
            print(f"  first node: {node.name}")

            # 测试 get_node_detail
            node_detail = tasker.get_node_detail(node.node_id)
            if node_detail:
                print(f"  node_detail: name={node_detail.name}, completed={node_detail.completed}")

            # 测试 get_recognition_detail
            if node.recognition:
                reco_detail = tasker.get_recognition_detail(node.recognition.reco_id)
                if reco_detail:
                    print(f"  reco_detail: name={reco_detail.name}, algorithm={reco_detail.algorithm}")

            # 测试 get_action_detail
            if node.action:
                action_detail = tasker.get_action_detail(node.action.action_id)
                if action_detail:
                    print(f"  action_detail: name={action_detail.name}, success={action_detail.success}")
    else:
        print("pipeline failed")
        raise RuntimeError("pipeline failed")

    # ============================================================
    # 断开连接
    # ============================================================
    if not agent.disconnect():
        print("failed to disconnect")
        exit(1)
    print("agent.disconnect() succeeded")
    child_process.wait(timeout=10)
    assert child_process.returncode == 0

    # 验证断开连接后的状态
    print(f"agent.connected after disconnect: {agent.connected}")

    print("\n" + "=" * 50)
    print(f"{scenario} passed!")
    print("=" * 50)


def run_numeric_identifier_flow():
    port = reserve_tcp_port()
    agent = AgentClient(str(port))
    run_tcp_flow(agent, agent.identifier, scenario="numeric identifier flow")


def run_numeric_identifier_flow_with_retry(max_attempts: int = 5):
    script = Path(__file__).resolve()

    for attempt in range(1, max_attempts + 1):
        # AgentClient(identifier) 需要预先选择一个非零空闲端口。
        # 这一步如果刚好与其他进程竞争失败，放到子进程里重试能避免整个测试进程直接失败。
        result = subprocess.run(
            [
                sys.executable,
                str(script),
                str(binding_dir),
                str(install_dir),
                NUMERIC_IDENTIFIER_FLAG,
            ],
            check=False,
        )
        if result.returncode == 0:
            return

        print(f"numeric identifier flow attempt {attempt}/{max_attempts} failed with exit code {result.returncode}")

    raise RuntimeError("numeric identifier flow failed after retries")


def api_test():
    # ============================================================
    # AgentClient TCP API 测试: 显式 create_tcp
    # ============================================================
    agent = AgentClient.create_tcp(0)
    run_tcp_flow(
        agent,
        agent.identifier,
        scenario="create_tcp flow",
        test_startup_failures=True,
    )

    # ============================================================
    # AgentClient TCP API 测试: 纯数字 identifier 自动走 TCP
    # ============================================================
    run_numeric_identifier_flow_with_retry()

    print("\n" + "=" * 50)
    print("All agent TCP tests passed!")
    print("=" * 50)


if __name__ == "__main__":
    print(f"AgentClient (TCP) MaaFw Version: {Library.version()}")

    Toolkit.init_option(install_dir / "bin")

    if len(sys.argv) >= 4 and sys.argv[3] == NUMERIC_IDENTIFIER_FLAG:
        run_numeric_identifier_flow()
        sys.exit(0)

    api_test()
