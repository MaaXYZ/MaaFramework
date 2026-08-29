"""
AgentClient 对端已死时的 disconnect 测试

测试范围:
1. AgentClient: 连接建立后杀死 AgentServer 子进程
2. AgentClient::disconnect: 对端已死时应跳过关闭握手并在有界时间内返回
   （alive_for 探活 5s 上限，而非无限阻塞在发送等待上）
"""

import atexit
import os
from pathlib import Path
import subprocess
import sys
import time

if len(sys.argv) < 3:
    print("Usage: python agent_dead_server_test.py <binding_dir> <install_dir>")
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

# alive_for 有界探活 5s + 余量
DISCONNECT_TIME_LIMIT = 7.0


def api_test():
    resource = Resource()
    resource.post_bundle(
        install_dir / "test" / "PipelineSmoking" / "resource"
    ).wait()
    if not resource.loaded:
        print("failed to load resource")
        exit(1)

    dbg_controller = DbgController(
        install_dir / "test" / "PipelineSmoking" / "Screenshot",
    )
    dbg_controller.post_connection().wait()

    tasker = Tasker()
    tasker.bind(resource, dbg_controller)
    if not tasker.inited:
        print("failed to init tasker")
        exit(1)

    agent = AgentClient()
    socket_id = agent.identifier
    if not agent.bind(resource):
        print("failed to bind resource")
        exit(1)
    if not agent.register_sink(resource, dbg_controller, tasker):
        print("failed to register sink")
        exit(1)

    child_proc = subprocess.Popen(
        [
            sys.executable,
            str(Path(__file__).parent / "agent_child_test.py"),
            str(binding_dir),
            str(install_dir),
            socket_id,
        ],
    )

    # 测试中途失败时回收子进程，避免残留
    def _kill_child():
        if child_proc.poll() is None:
            child_proc.kill()
        child_proc.wait()

    atexit.register(_kill_child)

    if not agent.connect():
        print("failed to connect to agent server")
        exit(1)
    print("agent.connect() succeeded")

    # ============================================================
    # 杀死服务端子进程，模拟 agent 进程崩溃
    # ============================================================
    child_proc.kill()
    child_proc.wait()
    time.sleep(1)  # 给 ZMQ 时间感知对端消失
    print("agent server killed, calling disconnect()...")

    # 残余路径保险：若 ZMQ 未及时感知死亡，alive_for 可能因陈旧 POLLOUT
    # 通过，握手 recv 将等待响应——设置有限超时使该路径有界失败（≈10s，
    # 触发下方 7s 断言）而非无限挂死拖垮 CI
    agent.set_timeout(10_000)

    # ============================================================
    # disconnect 应在有限时间内返回（alive_for 探活上限 5s + 余量），
    # 而非无限阻塞在关闭握手的发送等待上
    # ============================================================
    t0 = time.time()
    if not agent.disconnect():
        print("failed to disconnect")
        exit(1)
    elapsed = time.time() - t0
    print(f"agent.disconnect() returned in {elapsed:.2f}s")

    assert elapsed < DISCONNECT_TIME_LIMIT, (
        f"disconnect took {elapsed:.2f}s on dead server, "
        f"expected < {DISCONNECT_TIME_LIMIT}s (bounded alive_for)"
    )
    print("disconnect on dead server is bounded, test passed")


if __name__ == "__main__":
    print(f"AgentClient (dead server) MaaFw Version: {Library.version()}")

    Toolkit.init_option(install_dir / "bin")

    api_test()
