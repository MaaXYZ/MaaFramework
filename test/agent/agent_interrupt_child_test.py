"""
AgentServer 可中断测试（子进程侧）

模拟客户端进程消失（不发 ShutDownRequest）后，宿主本地调用 ShutDown 中断
阻塞中的 Join：start_up → 3 秒后本地 shut_down → join 计时返回。
验证消息循环的 recv 可被 ShutDown 中断（修复前默认无限超时下 Join 永久阻塞）。
"""

import os
from pathlib import Path
import sys
import threading
import time

if len(sys.argv) < 4:
    print("Call agent_interrupt_main_test.py instead of this file.")
    sys.exit(1)

binding_dir = Path(sys.argv[1]).resolve()
install_dir = Path(sys.argv[2]).resolve()

os.environ["MAAFW_BINARY_PATH"] = str(f"{install_dir}/bin")

if str(binding_dir) not in sys.path:
    sys.path.insert(0, str(binding_dir))

from maa.agent.agent_server import AgentServer
from maa.tasker import Tasker
from maa.library import Library

# 本地 ShutDown 在连接 3 秒后触发；join 随 poll 的 1 秒粒度检查尽快返回
LOCAL_SHUTDOWN_DELAY = 3.0
JOIN_TIME_LIMIT = 15.0


def main():
    socket_id = sys.argv[-1]

    AgentServer.start_up(socket_id)

    def local_shutdown():
        time.sleep(LOCAL_SHUTDOWN_DELAY)
        print("calling local AgentServer.shut_down()...")
        AgentServer.shut_down()

    # 非 daemon：join 该线程后再退出，避免解释器关闭时打断半执行的 shut_down
    shutdown_thread = threading.Thread(target=local_shutdown)
    shutdown_thread.start()

    t0 = time.time()
    AgentServer.join()
    elapsed = time.time() - t0
    print(f"AgentServer.join() returned after {elapsed:.2f}s")

    # join 能返回即证明阻塞的 recv 可被本地 ShutDown 中断（修复前永久挂死）
    assert elapsed < JOIN_TIME_LIMIT, (
        f"join took {elapsed:.2f}s, expected < {JOIN_TIME_LIMIT}s "
        f"(interruptible by local shut_down)"
    )
    # 下限：本场景下 join 不可能合法地早于本地 ShutDown 定时返回
    # （对端静默、无任何报文、默认无限超时——提前返回意味着循环异常退出）
    assert elapsed >= LOCAL_SHUTDOWN_DELAY, (
        f"join returned in {elapsed:.2f}s, earlier than the {LOCAL_SHUTDOWN_DELAY}s "
        f"local shut_down timer — loop likely exited on error, not interruption"
    )
    print("interruptible join OK")

    shutdown_thread.join(timeout=10)


if __name__ == "__main__":
    print(f"AgentServer (interrupt) MaaFw Version: {Library.version()}")

    Tasker.set_log_dir(install_dir / "bin" / "debug")

    main()
