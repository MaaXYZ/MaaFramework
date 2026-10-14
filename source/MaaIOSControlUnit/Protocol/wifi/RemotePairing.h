#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "wifi/Crypto.h"
#include "wifi/Rppairing.h"

namespace maa::ios::wifi
{

/// 设备 RemotePairing 端口的兜底值。端口本来是 mDNS SRV 里带的（这台 iOS 27 上
/// 观察到的一直是 49152），没有广播可问时才用它。
constexpr uint16_t kAdvertisedPortFallback = 49152;

/// 请设备开一个 **TCP** 隧道监听端口，返回端口号。
///
/// 前提：`channel` 已经 pair-verify 成功（主密钥装好了），`tunnel_key` 就是那一步
/// 的 X25519 共享密钥——TCP 这条路把它**原样**当 TLS-PSK 的密钥发过去，所以这把
/// 钥匙同时出现在两个地方：RemoteXPC 通道里（加密请求的对称密钥）和隧道的 TLS 里
/// （PSK）。QUIC 那条不是这样（它用一张自签 RSA 证书，且 iOS 18.2 之后被删了）。
///
/// 值得留意的一条现场症状：在 USB 那条 `remotepairingdeviced.lockdown` 控制面上调
/// 这个函数会被拒（"Tunnel listener creator not set"）——那条面只负责配对，
/// `allowsIncomingTunnelConnections` 报的就是 false。
std::optional<uint16_t> request_tcp_listener(Rppairing& channel, const Bytes& tunnel_key, std::string& err);

} // namespace maa::ios::wifi
