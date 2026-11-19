"""
AgentClient 端测试

测试范围:
1. AgentClient: 创建、绑定、连接管理、超时设置
2. AgentClient: custom_recognition_list、custom_action_list
3. 与 AgentServer 的完整交互流程
"""

import os
from pathlib import Path
import subprocess
import sys
import threading
import time
from typing import Optional

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


CONCURRENT_CREATE_FLAG = "--concurrent-create"
CANARY_CREATE_FLAG = "--canary-create"
CONCURRENT_CREATE_THREADS = 8
CONCURRENT_CREATE_TIMEOUT_SECONDS = 20


def run_canary_create() -> int:
    """子进程入口：单独探一次当前环境能不能创建 AgentClient。

    用于把「环境不支持」和「并发创建坏了」区分开，避免全部失败时测试静默通过。
    """
    try:
        AgentClient.create_tcp(0)
    except RuntimeError as exc:
        print(f"agent channel unavailable in this environment: {exc}")
        return 3
    print("agent channel available")
    return 0


def run_concurrent_create() -> int:
    """子进程入口：并发做首次 AgentClient 创建，验证探测不会互相卡住。

    调用方已用 canary 确认当前环境能创建 AgentClient，所以这里任何一个线程失败
    都视为回归，不再把失败当成预期。
    """
    identifiers: list[Optional[str]] = [None] * CONCURRENT_CREATE_THREADS
    errors: list[BaseException] = []
    # 起跑线：否则首个线程可能在其它线程进入前就把探测结果缓存掉，测不到并发路径
    barrier = threading.Barrier(CONCURRENT_CREATE_THREADS)

    def worker(index: int) -> None:
        try:
            barrier.wait()
            client = AgentClient.create_tcp(0)
            identifiers[index] = client.identifier
        except BaseException as exc:  # noqa: BLE001
            errors.append(exc)

    threads = [
        threading.Thread(target=worker, args=(index,), daemon=True)
        for index in range(CONCURRENT_CREATE_THREADS)
    ]
    start = time.monotonic()
    for thread in threads:
        thread.start()
    for thread in threads:
        thread.join(timeout=CONCURRENT_CREATE_TIMEOUT_SECONDS)
    elapsed = time.monotonic() - start

    stuck = [index for index, thread in enumerate(threads) if thread.is_alive()]
    if stuck:
        print(f"concurrent create stuck: threads={stuck} elapsed={elapsed:.1f}s")
        return 1
    if errors:
        print(f"concurrent create raised unexpected errors: {errors[0]!r}")
        return 1

    created = [identifier for identifier in identifiers if identifier]
    if len(created) != CONCURRENT_CREATE_THREADS:
        print(f"concurrent create returned partial results: {identifiers}")
        return 1

    print(
        f"concurrent create ok: threads={CONCURRENT_CREATE_THREADS} "
        f"created={len(created)} elapsed={elapsed * 1000:.0f}ms"
    )
    return 0


class ConflictingAction(CustomAction):
    def run(self, context, argv):
        return True


def run_child(flag: str, timeout_seconds: float) -> subprocess.CompletedProcess:
    return subprocess.run(
        [
            sys.executable,
            str(Path(__file__).resolve()),
            str(binding_dir),
            str(install_dir),
            flag,
        ],
        timeout=timeout_seconds,
        check=False,
    )


def agent_channel_available() -> bool:
    """新起一个进程探一次环境；只有它能建出 AgentClient 时才继续做并发断言。"""
    try:
        result = run_child(CANARY_CREATE_FLAG, CONCURRENT_CREATE_TIMEOUT_SECONDS)
    except subprocess.TimeoutExpired:
        raise AssertionError("agent client canary hung") from None

    if result.returncode == 3:
        return False
    assert result.returncode == 0, "agent client canary failed unexpectedly"
    return True


def run_concurrent_create_in_child() -> None:
    """首次创建必须在全新进程里发生，所以用子进程跑，父进程只负责限时等待。"""
    if not agent_channel_available():
        print("concurrent create skipped: this environment cannot create an agent client")
        return

    try:
        result = subprocess.run(
            [
                sys.executable,
                str(Path(__file__).resolve()),
                str(binding_dir),
                str(install_dir),
                CONCURRENT_CREATE_FLAG,
            ],
            timeout=CONCURRENT_CREATE_TIMEOUT_SECONDS * 2,
            check=False,
        )
    except subprocess.TimeoutExpired:
        raise AssertionError("concurrent agent client creation hung") from None

    assert result.returncode == 0, "concurrent agent client creation failed"


def api_test():
    # ============================================================
    # 并发首次创建回归（必须在本次进程首次创建之前跑）
    # ============================================================
    run_concurrent_create_in_child()

    # ============================================================
    # 创建并初始化 Resource, Controller, Tasker
    # ============================================================
    resource = Resource()
    print(f"resource: {resource}")

    # 加载资源
    resource_load = resource.post_bundle(
        install_dir / "test" / "PipelineSmoking" / "resource"
    ).wait()
    print(f"resource.loaded: {resource.loaded}")

    dbg_controller = DbgController(
        install_dir / "test" / "PipelineSmoking" / "Screenshot",
    )
    print(f"controller: {dbg_controller}")
    dbg_controller.post_connection().wait()

    assert Tasker.set_reco_image_raw(True)
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
        str(resource_load.job_id),
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
        assert "MultiRec" not in resource.custom_recognition_list, "agent registrations should be rolled back"
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

    if len(sys.argv) >= 4 and sys.argv[3] == CANARY_CREATE_FLAG:
        sys.exit(run_canary_create())

    if len(sys.argv) >= 4 and sys.argv[3] == CONCURRENT_CREATE_FLAG:
        sys.exit(run_concurrent_create())

    api_test()
