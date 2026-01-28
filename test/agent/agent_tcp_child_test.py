"""
AgentServer TCP 端测试

与 agent_child_test.py 完全相同，AgentServer 会自动检测 identifier 是否为端口号并使用 TCP 模式。
此文件仅为便于区分测试类型而创建。
"""

# 直接复用 agent_child_test.py 的代码
from agent_child_test import *

if __name__ == "__main__":
    print(f"AgentServer (TCP) MaaFw Version: {Library.version()}")

    Toolkit.init_option(install_dir / "bin")

    main()
