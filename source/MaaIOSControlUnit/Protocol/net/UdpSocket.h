#pragma once
#include "net/Stack.h"
#include <memory>

namespace maa::ios::net
{
// lwIP UDP 的同步数据报接口。PCB 只在核心线程访问，应用线程等待接收队列。
// 队列最多 4096 包、16 MiB，满时丢最老的数据；Stack 必须比本端点活得更久。
class UdpSocket
{
public:
    UdpSocket(Stack& stack, uint16_t local_port);
    ~UdpSocket();
    UdpSocket(const UdpSocket&) = delete;
    UdpSocket& operator=(const UdpSocket&) = delete;
    bool bind(std::string& err);
    bool send(const std::vector<uint8_t>& payload, uint16_t peer_port, std::string& err);
    // 保留数据报边界，只接收配置的对端地址，并返回源端口；超时不会关闭端点。
    bool recv(std::vector<uint8_t>& payload, uint16_t& peer_port, int timeout_ms, std::string& err);
    uint16_t local_port() const;
    // buffered 统计待取的数据报数，dropped 统计队列淘汰及分配失败的次数。
    size_t buffered() const;
    size_t dropped() const;

private:
    struct Impl;
    std::shared_ptr<Impl> impl_;
};
} // namespace maa::ios::net
