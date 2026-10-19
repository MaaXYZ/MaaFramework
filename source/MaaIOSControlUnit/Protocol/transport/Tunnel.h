#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "transport/PacketIo.h"
#include "transport/TlsChannel.h"
#include "transport/Usbmux.h"

namespace maa::ios::transport
{

/// CoreDeviceProxy 隧道握手后的参数。
struct TunnelParams
{
    std::string client_address; ///< 本机在隧道内的 IPv6
    std::string server_address; ///< 设备在隧道内的 IPv6
    uint16_t rsd_port = 0;      ///< 隧道内 RSD 监听端口
    uint16_t mtu = 0;
};

/// CoreDeviceProxy 数据包隧道。
///
/// 帧格式：
///   控制帧 = "CDTunnel"(8B magic) + u16 BE 长度 + JSON body
///   握手后线上是**裸 IPv6 包**：读时先取 40 字节 IPv6 头，用头内
///   bytes[4:5] 的 u16 BE payload 长度再取 body；写时不带任何帧头。
///
/// 一个必须遵守的约束：每个 IPv6 包要单独一次 write()，**不能合并写**。
/// CoreDeviceProxy 的转发路径对读边界敏感，合并写入可能破坏包边界。
class PacketTunnel : public PacketIo
{
public:
    PacketTunnel() = default;
    PacketTunnel(PacketTunnel&&) noexcept;
    PacketTunnel& operator=(PacketTunnel&&) noexcept;
    PacketTunnel(const PacketTunnel&) = delete;
    PacketTunnel& operator=(const PacketTunnel&) = delete;

    /// proxy 来自 lockdown StartService(CoreDeviceProxy)。
    ///
    /// use_tls=true 时，先用配对证书建立 TLS，再发送 CDTunnel 控制帧。
    static std::optional<PacketTunnel>
        establish(uint32_t device_id, uint16_t proxy_port, const PemIdentity& identity, bool use_tls, std::string& err);

    /// Wi-Fi 那条路：连接是普通的局域网 TCP，加密是 TLS-PSK（密钥来自 pair-verify），
    /// 之后的 CDTunnel 控制帧与 USB 那条**一模一样**，所以复用同一个对象、同一套
    /// 隧道内用户态栈。
    static std::optional<PacketTunnel> establish_psk(Socket&& sock, const std::vector<uint8_t>& psk, std::string& err);

    static std::optional<PacketTunnel> establish_bounded(
        uint32_t device_id,
        uint16_t proxy_port,
        const PemIdentity& identity,
        bool use_tls,
        const IoBudget& budget,
        std::string& err);
    static std::optional<PacketTunnel>
        establish_psk_bounded(Socket&& sock, const std::vector<uint8_t>& psk, const IoBudget& budget, std::string& err);

    [[nodiscard]] const TunnelParams& params() const { return params_; }

    [[nodiscard]] bool valid() const { return sock_.valid(); }

    /// 发一个完整 IPv6 包（一次 write，不合并）。
    bool send_ipv6(const uint8_t* packet, size_t len, std::string& err) override;
    /// 收一个完整 IPv6 包。
    bool recv_ipv6(std::vector<uint8_t>& out, std::string& err) override;

    /// 是否有数据可读（TLS 下要先看 SSL 内部缓冲，否则会把已解密的字节等丢）。
    /// timed_out 用于区分正常的等待超时与传输错误。
    bool wait_readable(int ms, std::string& err, bool* timed_out = nullptr) override;

    uint16_t mtu() const override { return params_.mtu; }

    void shutdown() override { sock_.interrupt(); }

private:
    /// 发 clientHandshakeRequest 并解析回复。两条路（证书 TLS / PSK）共用这一段。
    bool client_handshake(std::string& err);

    bool write_all(const void* data, size_t len, std::string& err);
    bool read_all(void* data, size_t len, std::string& err);

    Socket sock_;
    TlsChannel tls_;
    TunnelParams params_;
};

} // namespace maa::ios::transport
