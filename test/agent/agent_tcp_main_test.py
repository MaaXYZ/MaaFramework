"""
AgentClient TCP 端测试

测试范围:
1. AgentClient TCP 模式: 创建、绑定、连接管理、超时设置
2. AgentClient: custom_recognition_list、custom_action_list
3. 与 AgentServer 的完整交互流程 (通过 TCP)
"""

import os
from pathlib import Path
import sys
import subprocess

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
from maa.controller import DbgController, MaaDbgControllerTypeEnum
from maa.tasker import Tasker
from maa.agent_client import AgentClient
from maa.toolkit import Toolkit


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

    # ============================================================
    # AgentClient TCP API 测试
    # ============================================================
    TCP_PORT = 23456  # 使用一个固定端口用于测试

    agent = AgentClient.create_tcp(TCP_PORT)
    print(f"agent (TCP): {agent}")

    # 测试 identifier (应该是端口号字符串)
    socket_id = agent.identifier
    print(f"agent.identifier: {socket_id}")
    if socket_id != str(TCP_PORT):
        print(f"unexpected identifier: {socket_id}, expected: {TCP_PORT}")
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
    # 启动 AgentServer 子进程 (TCP 模式)
    # ============================================================
    subprocess.Popen(
        [
            "python",
            str(Path(__file__).parent / "agent_tcp_child_test.py"),
            str(binding_dir),
            str(install_dir),
            socket_id,  # 传递端口号字符串
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

    # 验证断开连接后的状态
    print(f"agent.connected after disconnect: {agent.connected}")

    print("\n" + "=" * 50)
    print("All agent TCP tests passed!")
    print("=" * 50)


if __name__ == "__main__":
    print(f"AgentClient (TCP) MaaFw Version: {Library.version()}")

    Toolkit.init_option(install_dir / "bin")

    api_test()
