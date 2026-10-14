#pragma once

#include <openssl/ssl.h>

#include <memory>
#include <string>
#include <vector>

#include "transport/IoBudget.h"
#include "transport/Socket.h"

namespace maa::ios::transport
{

/// 配对记录里用于双向 TLS 的三块 PEM 材料。
struct PemIdentity
{
    std::vector<uint8_t> host_cert;
    std::vector<uint8_t> host_key;
    std::vector<uint8_t> root_cert;

    [[nodiscard]] bool complete() const { return !host_cert.empty() && !host_key.empty() && !root_cert.empty(); }
};

/// 把一条已连接的 socket 升级为 lockdown 风格的 TLS 通道。
///
/// lockdown 主会话和 EnableServiceSSL 的服务连接（如 CoreDeviceProxy）用的是
/// 同一套证书，所以两边共用这里，避免复制一份容易写错的 TLS 建立代码。
class TlsChannel
{
public:
    TlsChannel();
    ~TlsChannel();
    TlsChannel(TlsChannel&&) noexcept;
    TlsChannel& operator=(TlsChannel&&) noexcept;
    TlsChannel(const TlsChannel&) = delete;
    TlsChannel& operator=(const TlsChannel&) = delete;

    /// 仅成功握手后持有 TLS 句柄；再次握手失败会清空原连接和候选状态。
    [[nodiscard]] bool valid() const { return ssl_ != nullptr; }

    [[nodiscard]] SSL* handle() const { return ssl_; }

    /// 借用 sock，不取得 socket 所有权；返回 false 时 handle() 为 nullptr。
    bool handshake(Socket& sock, const PemIdentity& id, std::string& err);

    /// 使用 pair-verify 派生的共享密钥建立 TLS 1.2 PSK 通道，身份为空字符串。
    /// 已验证的设备隧道监听器使用 TLS 1.2 PSK 套件；TLS 1.3 external PSK
    /// 是不同机制，当前未做设备适配。该路径不发送或校验证书，以 PSK 认证。
    bool handshake_psk(Socket& sock, const std::vector<uint8_t>& psk, std::string& err);

    bool handshake_bounded(Socket& sock, const PemIdentity& id, const IoBudget& budget, std::string& err);
    bool handshake_psk_bounded(Socket& sock, const std::vector<uint8_t>& psk, const IoBudget& budget, std::string& err);

private:
    void release();
    SSL_CTX* ctx_ = nullptr;
    SSL* ssl_ = nullptr;
    // SSL ex_data 持有此 vector 的地址。独立存储保证移动通道时地址不变，
    // 并且在 SSL_free 后才释放，避免回调访问移动来源或已销毁的对象。
    std::unique_ptr<std::vector<uint8_t>> psk_;
};

} // namespace maa::ios::transport
