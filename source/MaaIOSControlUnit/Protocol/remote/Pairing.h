#pragma once

#include "wifi/PairRecord.h"

#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace maa::ios::transport
{
struct DeviceRecord;
}

namespace maa::ios::wifi
{
struct PairRecord;
struct PairSetupResult;
struct PairVerifyResult;
}

namespace maa::ios::remote
{

struct UsbPairingOptions
{
    std::string udid; ///< 空值只选择唯一 USB 设备；不选择 usbmux Network 条目。
    std::vector<wifi::PairRecord> existing_records;
    int timeout_ms = 120000;
    /// 允许对不完整或已被设备拒绝的记录重新配对。已有可用记录仍优先复用，
    /// 传输或身份校验失败不会触发重新配对。记录属于另一 UDID 时始终拒绝覆盖。
    bool allow_repair = false;
    std::function<void(std::string_view)> progress;
    /// May be polled from an I/O cancellation monitor; must be thread safe.
    std::function<bool()> should_cancel;
};

struct PairingResult
{
    bool ok = false;
    bool reused = false;
    std::string udid;
    std::optional<wifi::PairRecord> record;
    std::string error;
};

/// 经可信 USB 入口建立供无线连接使用的 RemotePairing 记录。
/// 已有记录先 PairVerify；新配对要求设备确认，并由 pair_setup 校验 SRP/M6 身份。
/// 使用新服务连接 PairVerify 成功后才向调用方返回凭据；本流程不读写文件。
/// 设备已执行的配对不能回滚；调用方负责保存已返回的凭据。
/// timeout_ms 范围为 1..300000，限制每次配对回复等待（包括用户确认），
/// 不表示整个流程的总 deadline；每次 USB 隧道和 RSD 准备有独立 30 秒预算，可通过 should_cancel 中断。
PairingResult pair_usb_remote(const UsbPairingOptions& options = { });

namespace detail
{

/// 纯设备选择边界。要求匹配的 USB 候选唯一，保留原始 UDID 和 device_id。
std::optional<transport::DeviceRecord>
    select_pairing_usb_device(const std::vector<transport::DeviceRecord>& devices, std::string_view udid, std::string& error);

/// 只注入两项协议操作以测试工作流的失败门控。生产实现每次打开独立 USB
/// RemoteXPC 连接，复用 Device 建立隧道，不给调用方暴露密钥或底层资源所有权。
struct PairingWorkflowOperations
{
    std::function<wifi::PairVerifyResult(const wifi::PairRecord&, std::string&)> verify;
    std::function<wifi::PairSetupResult(std::string_view host_identifier, std::string_view hostname, std::string_view udid, std::string&)>
        setup;
};

/// 执行内存凭据复用/重新配对决策，验证后返回；不枚举设备或访问文件。
/// selected_udid 必须由上述 USB 选择边界取得，hostname 用于新主机标识生成。
/// 离线测试只使用合成记录和回调，协议密码学校验由 pair_setup/pair_verify 负责。
PairingResult run_usb_pairing_workflow(
    std::string_view selected_udid,
    std::string_view hostname,
    const UsbPairingOptions& options,
    const PairingWorkflowOperations& operations);

} // namespace detail
} // namespace maa::ios::remote
