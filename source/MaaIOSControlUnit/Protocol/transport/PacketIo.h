#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace maa::ios::transport
{
// 完整 IPv6 包的 I/O 边界。shutdown 必须中断正在进行的读写，并禁止后续 I/O。
class PacketIo
{
public:
    virtual ~PacketIo() = default;
    virtual bool send_ipv6(const uint8_t*, size_t, std::string&) = 0;
    virtual bool recv_ipv6(std::vector<uint8_t>&, std::string&) = 0;
    virtual bool wait_readable(int, std::string&, bool* timed_out = nullptr) = 0;
    virtual uint16_t mtu() const = 0;
    virtual void shutdown() = 0;
};
} // namespace maa::ios::transport
