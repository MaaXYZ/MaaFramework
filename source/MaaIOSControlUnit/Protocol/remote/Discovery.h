#pragma once

#include "wifi/PairRecord.h"

#include <chrono>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace maa::ios::transport
{
struct DeviceRecord;
}

namespace maa::ios::wifi
{
struct PairRecord;
}

namespace maa::ios::wifi::mdns
{
struct Advertisement;
}

namespace maa::ios::remote
{

enum class DiscoveryTransport
{
    usbmux,
    remote_pairing
};
enum class DiscoveryPairing
{
    unmatched,
    ready,
    needs_pairing,
    identifier_hint,
    ambiguous,
    invalid_advertisement,
    crypto_error,
};

struct DiscoveryCandidate
{
    DiscoveryTransport transport = DiscoveryTransport::usbmux;
    std::string connection_type;
    uint32_t device_id = 0;
    std::string address;
    uint16_t port = 0;
    uint32_t interface_index = 0;
    std::string interface_name;
    std::string instance;
    std::string identifier;
    DiscoveryPairing pairing = DiscoveryPairing::unmatched;
    bool operator==(const DiscoveryCandidate&) const = default;
};

struct DiscoveredDevice
{
    /// USB 原始 UDID 或唯一 authTag 匹配记录中的原始 UDID；未知无线设备留空。
    std::string udid;
    std::string name;
    std::vector<DiscoveryCandidate> candidates;
};

struct DiscoveryOptions
{
    std::chrono::milliseconds timeout { 3000 };
    /// 留空表示不取消。回调应快速返回且不抛异常；共享状态由调用方同步。
    std::function<bool()> should_cancel;
    std::vector<wifi::PairRecord> pairing_records;
    bool include_usb = true;
    bool include_wifi = true;
    /// 独立于无线扫描的 usbmux 枚举预算（0..60 秒），涵盖连接及整个请求/回复。
    std::chrono::milliseconds usb_timeout { 1000 };
};

struct DiscoveryResult
{
    std::vector<DiscoveredDevice> devices;
    std::vector<std::string> warnings;
    bool usb_available = false;
    bool wifi_available = false;
    bool cancelled = false;
};

/// 返回本次枚举的快照，不执行配对、认证、连接设备服务或启动媒体会话。
/// timeout 是无线扫描时间（0..60 秒）；零值跳过无线扫描及配对文件读取，
/// USB 枚举仍由 include_usb 控制，使用独立的 usb_timeout 绝对预算。
/// 预取消时不访问任何来源；扫描中取消保留已收集的结果。
DiscoveryResult discover_devices(const DiscoveryOptions& options = { });

namespace detail
{

/// 纯合并边界。只以唯一 authTag 匹配记录的真实 UDID 关联 USB 和无线候选；
/// 广播标识相等仅是展示线索。保留不同地址、实例和接口，去除完全相同的候选。
/// 返回值的来源可用性标志保持 false，实际扫描器负责设置。
DiscoveryResult merge_discovery(
    const std::vector<transport::DeviceRecord>& usb_records,
    const std::vector<wifi::mdns::Advertisement>& advertisements,
    const std::vector<wifi::PairRecord>& pair_records);

/// 有界读取普通 remote-*.pair 文件，跳过符号链接；最多检查 4096 个目录项、
/// 读取 256 份各不超过 16 KiB 的记录。坏文件不阻止其他记录，warning 不含文件名、
/// 密钥、记录内容或设备标识。缺失目录返回空列表且不产生 warning。
/// 单独提供这条内部边界以便离线检查加载行为，不创建或修改任何文件。
std::vector<wifi::PairRecord> load_discovery_records(
    const std::string& directory,
    const std::function<bool()>& should_cancel,
    std::vector<std::string>& warnings,
    bool& cancelled);

} // namespace detail
} // namespace maa::ios::remote
