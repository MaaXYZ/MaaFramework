#pragma once

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "plist/Plist.h"
#include "transport/TlsChannel.h"
#include "transport/Usbmux.h"

namespace maa::ios::transport
{

/// lockdown 客户端：连接、建立配对 session（含双向 TLS）、起服务端口。
///
/// StartService 前先执行 StartSession 和 TLS 握手，避免 SessionInactive。
///
/// 帧格式是 [len:u32 大端][XML plist]——注意字节序和 usbmuxd 帧（小端）相反。
class Lockdown
{
public:
    Lockdown();
    ~Lockdown();
    Lockdown(Lockdown&&) noexcept;
    Lockdown& operator=(Lockdown&&) noexcept;
    Lockdown(const Lockdown&) = delete;
    Lockdown& operator=(const Lockdown&) = delete;

    /// 完整建立：连 usbmuxd -> 读配对记录 -> Connect(62078) -> StartSession -> TLS。
    static std::optional<Lockdown> establish_bounded(
        uint32_t device_id,
        std::string_view udid,
        std::chrono::milliseconds timeout,
        const std::function<bool()>& cancelled,
        std::string& err);

    static std::optional<Lockdown> establish_bounded(uint32_t device_id, std::string_view udid, const IoBudget& budget, std::string& err);

    /// 发一个 lockdown 请求并等回复。自动补 Label 字段。
    bool request(const plist::Value& req, plist::Value& reply, std::string& err);
    bool request_bounded(
        const plist::Value&,
        plist::Value&,
        std::chrono::milliseconds timeout,
        const std::function<bool()>& cancelled,
        std::string& err);

    /// 起服务并返回可 Connect 的端口。
    ///
    /// EnableServiceSSL=true 时，后续服务连接也需使用配对证书建立 TLS。
    struct ServiceEndpoint
    {
        uint16_t port = 0;
        bool requires_tls = false;
    };

    std::optional<ServiceEndpoint> start_service(std::string_view name, std::string& err);

    std::optional<ServiceEndpoint> start_service_bounded(std::string_view name, const IoBudget& budget, std::string& err);

    /// 配对记录里的 TLS 材料。开启 EnableServiceSSL 的服务连接要用同一套。
    [[nodiscard]] const PemIdentity& identity() const { return identity_; }

    /// 本机在这台设备上的配对身份（一个 36 字符的 UUID 文本）。
    ///
    /// 它是现成的、跨进程跨重启都稳定的主机标识，正好拿来当 RemoteXPC 握手里的
    /// peer UUID——那个 UUID 一旦变了，设备会把整台机器重新 attach 一遍并关掉
    /// 已公布的服务端口，所以「每次运行都随机生成」是不能接受的写法。
    [[nodiscard]] const std::string& host_id() const { return host_id_; }

    [[nodiscard]] const std::string& session_id() const { return session_id_; }

    // Interrupt an in-flight I/O without transferring ownership. The caller
    // must join its watchdog before moving or destroying this session.
    void interrupt();

private:
    Socket sock_;
    TlsChannel tls_;
    PemIdentity identity_;
    std::string host_id_;
    std::string system_buid_;
    std::string session_id_;
    bool bounded_io_ = false;
};

} // namespace maa::ios::transport
