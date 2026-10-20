#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "plist/Plist.h"
#include "transport/Socket.h"

namespace maa::ios::transport
{

struct DeviceRecord
{
    uint32_t device_id = 0;
    std::string udid;            ///< Properties.SerialNumber
    std::string connection_type; ///< "USB" / "Network"
    uint32_t product_id = 0;

    [[nodiscard]] bool is_usb() const { return connection_type == "USB"; }
};

enum class UsbmuxDiscoveryStatus
{
    complete,
    unavailable,
    timed_out,
    cancelled
};

/// usbmux 客户端：macOS / Linux 使用 AF_UNIX，Windows 使用 AMDS 本地 TCP。
///
/// 帧格式（已有 macOS 设备记录）：
///   [len:u32 LE，含自身][version:u32=1][message:u32][tag:u32][payload]
/// plist 协议下 message 恒为 8(PLIST)，payload 是 XML plist 再加一个 NUL。
///
/// 连接约束：
///   - Connect 不能在发过 Listen 的连接上用，否则回 Number=5(NOT_CONNECTED)。
///     所以本类走 ListDevices + Connect 的同一条连接。
///   - PortNumber 必须是网络序（htons）。传主机序会回 Number=3(CONNREFUSED)。
class Usbmux
{
public:
    Usbmux() = default;
    Usbmux(Usbmux&&) noexcept;
    Usbmux& operator=(Usbmux&&) noexcept;
    Usbmux(const Usbmux&) = delete;
    Usbmux& operator=(const Usbmux&) = delete;

    /// 连接 usbmux 服务：macOS/Linux 为 /var/run/usbmuxd，Windows 为 127.0.0.1:27015。
    static std::optional<Usbmux> open(std::string& err);
    // Dedicated deadline/cancellation-aware connection for installation.
    // Existing discovery/pairing/media open/connect semantics are unchanged.
    static std::optional<Usbmux>
        open_for_service(std::chrono::milliseconds timeout, const std::function<bool()>& cancelled, std::string& err);
    std::optional<Socket> connect_for_service(
        uint32_t device_id,
        uint16_t port,
        std::chrono::milliseconds timeout,
        const std::function<bool()>& cancelled,
        std::string& err);

    bool list_devices(std::vector<DeviceRecord>& out, std::string& err);
    bool list_devices_for_service(
        std::vector<DeviceRecord>& out,
        std::chrono::milliseconds timeout,
        const std::function<bool()>& cancelled,
        std::string& err);

    /// 仅供设备发现使用的独立连接。connect/send/header/payload 共用绝对预算，
    /// socket 等待以最多 50 ms 切片检查取消；失败时 out 为空。err 仅用于系统或协议错误，
    /// 超时/取消由返回状态区分，不改变 open/list_devices 的既有阻塞语义。
    static UsbmuxDiscoveryStatus list_devices_for_discovery(
        std::vector<DeviceRecord>& out,
        std::chrono::milliseconds timeout,
        const std::function<bool()>& should_cancel,
        std::string& err);

    /// 把 socket 转发到 device_id:port。成功后本对象失效，返回的 Socket
    /// 就是到该端口的透明通道（lockdown 即接在这上面）。
    std::optional<Socket> connect(uint32_t device_id, uint16_t port, std::string& err);

    /// 读配对记录（bplist 字节）。
    ///
    /// 必须走 usbmuxd 而不是直接读文件：macOS 上 /var/db/lockdown 是 root-only，
    /// 而 usbmuxd 有权限代读。返回的 blob 里含私钥与证书，**绝不可写进日志**。
    bool read_pair_record(std::string_view udid, std::vector<uint8_t>& out, std::string& err);
    bool read_pair_record_for_service(
        std::string_view udid,
        std::vector<uint8_t>& out,
        std::chrono::milliseconds timeout,
        const std::function<bool()>& cancelled,
        std::string& err);

    [[nodiscard]] static std::string socket_path();

private:
    bool round_trip(const plist::Value& request, plist::Value& reply, std::string& err);
    bool round_trip_for_service(const plist::Value&, plist::Value&, std::chrono::milliseconds, const std::function<bool()>&, std::string&);

    Socket sock_;
    uint32_t tag_ = 1;
};

namespace detail
{
/// 内部测试边界：生产入口始终使用系统 usbmux 服务，不提供 endpoint 配置。
struct UsbmuxDiscoveryEndpoint
{
#ifdef _WIN32
    uint16_t port = 27015; // 仅 loopback TCP，不解析任意主机名。
#else
    std::string path = "/var/run/usbmuxd";
#endif
};

UsbmuxDiscoveryStatus list_usbmux_devices_at(
    const UsbmuxDiscoveryEndpoint& endpoint,
    std::vector<DeviceRecord>& out,
    std::chrono::milliseconds timeout,
    const std::function<bool()>& should_cancel,
    std::string& err);
} // namespace detail

} // namespace maa::ios::transport
