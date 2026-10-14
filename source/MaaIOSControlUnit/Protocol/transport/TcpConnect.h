#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "transport/IoBudget.h"
#include "transport/Usbmux.h"
#include <functional>
#include <vector>

namespace maa::ios::transport
{

/// 连一个普通的 TCP 服务端点（IPv4/IPv6 字面量或主机名）。
///
/// 这一条不属于 `Usbmux`：Wi-Fi 那条路上，与设备打交道的第一跳是局域网里的一个
/// 普通 TCP 端口，没有 usbmuxd 参与。分开写，也是为了让"哪些字节是走系统协议栈
/// 出去的、哪些是走我们自己那个隧道内用户态栈出去的"这件事在文件层次上看得出来。
///
/// `timeout_ms` 按每个候选地址限制 connect 等待，不包含 DNS 解析。给 <= 0 表示不超时（不建议：设备睡眠时 ARP 会静默
/// 丢包，无限等就变成一个看不出原因的卡死）。
std::optional<Socket> connect_tcp(const std::string& host, uint16_t port, int timeout_ms, std::string& err);

/// DNS and every address candidate share budget. Numeric addresses avoid DNS.
/// At most two outstanding hostname resolutions are allowed process-wide. A
/// timed-out system resolver may finish in the background; further hostname
/// attempts are rejected while both slots remain occupied.
std::optional<Socket> connect_tcp_bounded(const std::string& host, uint16_t port, const IoBudget& budget, std::string& err);

namespace detail
{
struct TcpAddress
{
    sockaddr_storage address { };
    socklen_t length = 0;
    int family = 0;
    int type = SOCK_STREAM;
    int protocol = 0;
};

// Offline test boundary. Resolver captures must own their state: they can
// outlive the call after cancellation, but never receive the caller's budget.
using TcpResolver = std::function<std::vector<TcpAddress>(const std::string&, uint16_t, std::string&)>;
std::optional<Socket> connect_tcp_bounded_with_resolver(
    const std::string& host,
    uint16_t port,
    const IoBudget& budget,
    std::string& err,
    TcpResolver resolver);
} // namespace detail

} // namespace maa::ios::transport
