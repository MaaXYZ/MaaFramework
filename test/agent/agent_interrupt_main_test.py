"""
AgentServer 可中断测试（主进程侧）

测试范围:
1. AgentClient 连接后直接退出（不发 ShutDownRequest），模拟客户端进程消失
2. AgentServer 侧（子进程）在本地 ShutDown 兜底下，阻塞中的 Join 应在
   有限时间内返回——验证消息循环 recv 的可中断性
   （修复前：默认无限超时下 Join 永久阻塞，agent 进程残留）
"""

import os
from pathlib import Path
import subprocess
import sys

if len(sys.argv) < 3:
    print("Usage: python agent_interrupt_main_test.py <binding_dir> <install_dir>")
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

# 子进程内部断言 join < 15s（3s 定时 + 1s poll 粒度 + 慢机余量）
CHILD_WAIT_TIMEOUT = 60


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
            str(Path(__file__).parent / "agent_interrupt_child_test.py"),
            str(binding_dir),
            str(install_dir),
            socket_id,
        ],
    )

    if not agent.connect():
        print("failed to connect to agent server")
        child_proc.kill()
        exit(1)
    print("agent.connect() succeeded")

    # ============================================================
    # 主进程不调用 disconnect、不发 ShutDownRequest，原地等待子进程——
    # 模拟客户端进程消失。子进程的 Join 只能靠其本地 ShutDown 兜底打断
    # ============================================================
    print("parent exits WITHOUT disconnect (simulating client vanishing)")

    try:
        child_proc.wait(timeout=CHILD_WAIT_TIMEOUT)
    except subprocess.TimeoutExpired:
        print(f"child process still alive after {CHILD_WAIT_TIMEOUT}s, join hung")
        child_proc.kill()
        exit(1)

    if child_proc.returncode != 0:
        print(f"agent child process exited with {child_proc.returncode}")
        exit(1)
    print("agent child process exited gracefully (interruptible join verified)")


if __name__ == "__main__":
    print(f"AgentClient (interrupt) MaaFw Version: {Library.version()}")

    Toolkit.init_option(install_dir / "bin")

    api_test()
