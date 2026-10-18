#pragma once

#include <optional>
#include <stop_token>
#include <string>
#include <string_view>
#include <vector>

#include "net/Stack.h"
#include "remote/Rsd.h"
#include "transport/IoBudget.h"
#include "transport/Lockdown.h"
#include "transport/Tunnel.h"
#include "transport/Usbmux.h"
#include "wifi/PairRecord.h"
#include "wifi/RemotePairing.h"

namespace maa::ios::remote
{

/// 根据 lockdown 错误选择连接提示，区分锁屏、未信任和服务不可用。
std::string proxy_failure_hint(std::string_view lockdown_error);

namespace detail
{
/// usbmux 的 USB/Network 条目按非空原始 UDID 归为同一设备。无 serial 时要求
/// 唯一设备，有 serial 时精确匹配；同一设备优先唯一 USB 条目。所选传输重复时
/// 返回错误，不任取一个 device_id。usb_only 先过滤非 USB 条目，错误只显示脱敏 UDID。
std::optional<transport::DeviceRecord> select_usbmux_device(
    const std::vector<transport::DeviceRecord>& records,
    std::string_view serial,
    std::string& error,
    bool usb_only = false);
} // namespace detail

/// 持有一次设备会话的配对连接、包隧道、IPv6 协议栈和 RSD 目录。
/// USB 经 lockdown/CoreDeviceProxy 建立隧道，Wi-Fi 经远程配对和 TLS-PSK 建立。
/// 服务连接及引用 Device 的媒体源、订阅者必须先于 Device 销毁。
/// 成员按逆序析构：先释放 RSD 连接，再停止 Stack，最后释放隧道及 lockdown。
class Device
{
public:
    Device() = default;
    Device(Device&&) noexcept;
    /// 覆盖旧会话前，调用方必须已停止并销毁借用它的服务连接、媒体源和订阅者。
    Device& operator=(Device&&) noexcept;
    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;
    ~Device();

    /// 读取 usbmux 当前列出的设备；失败时返回空列表并填写 err。
    static std::vector<transport::DeviceRecord> list(std::string& err);

    /// 经 usbmux 建立会话。udid 为空时自动选择唯一真实 UDID；同设备优先 USB，
    /// 多台设备时返回脱敏候选列表，同一传输条目重复时要求解决歧义后重试。
    /// usb_only 过滤 Network 条目，供首次远程配对等必须使用 USB 的操作调用。
    /// verbose 通过调试日志报告各阶段进展；失败时已取得的资源随局部对象释放。
    static std::optional<Device> establish(std::string_view udid, std::string& err, bool verbose = false, bool usb_only = false);
    static std::optional<Device> establish_bounded(
        std::string_view udid,
        const transport::IoBudget& budget,
        std::string& err,
        bool verbose = false,
        bool usb_only = false);

    /// 使用远程配对记录，经 pair-verify、设备监听端口和 TLS-PSK 建立局域网隧道。
    /// address/port 指向设备 RemotePairing 服务；隧道建立后复用同一套 Stack/RSD。
    /// 此路径不建立 lockdown 会话，不能提供依赖 lockdownd 的功能。
    static std::optional<Device> establish_wifi(
        const std::string& address,
        const wifi::PairRecord& record,
        std::string& err,
        bool verbose = false,
        uint16_t port = wifi::kAdvertisedPortFallback);
    static std::optional<Device> establish_wifi_bounded(
        const std::string& address,
        const wifi::PairRecord& record,
        const transport::IoBudget& budget,
        std::string& err,
        bool verbose = false,
        uint16_t port = wifi::kAdvertisedPortFallback);

    /// Abort startup I/O on this owned tunnel only. Caller must stop/join its
    /// watchdog before moving or destroying Device; never resume an aborted session.
    void interrupt_startup();

    /// 仅在 establish/establish_wifi 成功后访问 RSD 和隧道参数。
    [[nodiscard]] Rsd& rsd() { return *rsd_; }

    /// 隧道协商的本机/设备 IPv6 地址、RSD 端口及 MTU。
    [[nodiscard]] const transport::TunnelParams& tunnel_params() const { return tunnel_->params(); }

    [[nodiscard]] const Rsd& rsd() const { return *rsd_; }

    /// 隧道上的用户态协议栈，供端点、网络统计和调试使用；未建立时可能为空。
    [[nodiscard]] net::Stack* stack() { return stack_.get(); }

    [[nodiscard]] const std::string& udid() const { return udid_; }

    /// USB 路径保留 usbmux 的连接类型，局域网路径使用 WiFi。
    [[nodiscard]] const std::string& connection_type() const { return connection_type_; }

    /// 从 peer_info.Properties 读取字符串字段；缺失或类型不符时返回空字符串。
    [[nodiscard]] std::string property(std::string_view key) const;

    /// 按 RSD 目录建立独立服务连接；返回对象必须先于 Device 销毁。
    /// cancel 覆盖建连和返回连接的生命期，只关闭该服务的 TCP 端点。
    std::unique_ptr<ServiceConnection>
        connect(std::string_view service_name, std::string& err, bool verbose = false, std::stop_token cancel = { });

    /// 使用新服务连接调用一次 CoreDevice feature，返回前释放连接。
    /// timeout_ms 传给 invoke，不包含建立 TCP/RemoteXPC 连接所需的等待。
    bool feature(
        std::string_view service_name,
        std::string_view feature_identifier,
        std::string_view action_identifier,
        const xpc::Value& input,
        xpc::Value& output,
        std::string& err,
        bool verbose = false,
        int timeout_ms = 10000);

    /// 与 feature 相同，但区分成功、能力/业务错误及连接/传输错误，不自动重试。
    /// TransportError 只说明未取得有效回复，不能据此断定设备未执行请求。
    CallResult feature_call(
        std::string_view service_name,
        std::string_view feature_identifier,
        std::string_view action_identifier,
        const xpc::Value& input,
        xpc::Value& output,
        std::string& err,
        bool verbose,
        int timeout_ms);

private:
    /// USB/Wi-Fi 共用的隧道收尾：构造 Stack、启动隧道线程，再连接 RSD 读取目录。
    bool finish_session(
        transport::PacketTunnel&& tunnel,
        PeerIdentity identity,
        const transport::IoBudget& budget,
        bool verbose,
        std::string& err);

    std::string udid_;
    std::string connection_type_;
    std::optional<transport::Lockdown> lockdown_;
    /// Stack 引用 PacketTunnel，Rsd 引用 Stack；资源置于堆上，移动构造 Device
    /// 时保持被引用对象的地址。声明顺序保证 Rsd、Stack、隧道依次释放。
    std::unique_ptr<transport::PacketTunnel> tunnel_;
    std::unique_ptr<net::Stack> stack_;
    std::optional<Rsd> rsd_;
};

/// 长于 keep 的标识只显示尾部，前部替换为 ****；较短字符串原样返回。
/// 仅用于日志显示，不修改配对和协议使用的原始标识。
[[nodiscard]] std::string mask(std::string_view value, std::size_t keep = 4);

} // namespace maa::ios::remote
