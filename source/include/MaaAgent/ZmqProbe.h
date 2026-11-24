#pragma once

#include "Common/Conf.h"

MAA_AGENT_NS_BEGIN

// 当前进程是否还能创建 ZeroMQ socket。
// 在沙箱、AppContainer 或部分安全软件环境下 libzmq 会直接 abort（SEH，不是 C++ 异常），
// 调用方据此提前失败，避免把整个宿主进程带走。
bool zmq_socket_available();

MAA_AGENT_NS_END
