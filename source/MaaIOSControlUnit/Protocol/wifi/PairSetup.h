#pragma once

#include <string>
#include <string_view>

#include "wifi/PairRecord.h"
#include "wifi/Rppairing.h"
#include "json/Json.h"

namespace maa::ios::wifi
{

/// 由主机名生成 DNS 命名空间的 UUID v3，输出大写文本；MD5 不可用时返回空串。
/// pair-setup 的标识字段、M5 签名和后续 pair-verify 签名应使用同一字符串。
/// 此函数提供确定性生成方式；读取已有记录时应使用保存的标识，不重新生成。
std::string host_identifier_uuid3(std::string_view hostname);

/// 获取本机主机名，失败时返回空串；调用方需报错或提供稳定的替代标识。
std::string local_hostname();

struct PairSetupResult
{
    bool ok = false;
    /// 成功时生成含已校验设备长期身份的记录；保存由调用方负责，仍需核对
    /// 传入的设备标识与首次配对入口的信任来源。
    PairRecord record;
    /// 设备 handshake 响应体，包含 peerDeviceInfo 等信息，可供调用方查询协商能力。
    json::Value device_handshake;
    std::string error;
};

/// pair-setup 的入口参数。流程包含可选 verify 探测及 M1..M6。
/// 当前 SRP 用户名为 Pair-Setup、PIN 固定为 "000000"，不提供交互输入其他 PIN 的接口。
/// 已有设备记录显示不同控制面可能有不同确认流程；progress 用于报告中间状态。
/// 接入其他设备配置时需重新验证，不能假定所有 iOS/tvOS 都采用相同配对方式。
struct PairSetupOptions
{
    /// 是否先发送 attemptPairVerify=true 的 handshake 并执行 verify 探测；false
    /// 表示直接以 attemptPairVerify=false 开始 setup。已测 iOS 27 字节流入口存在
    /// verify 后切换 setup 被关闭连接的情况（docs §25.2/§25.3），开关保留两种入口。
    bool probe_verify_first = true;
    /// pairingData 的 kind，默认升级已有 lockdown 信任到远程配对。
    /// 已测 iOS 27 的字节流入口和隧道内 RemoteXPC 入口行为不同：后者接受
    /// setupManualPairing 并要求设备确认，免提示升级还受主机授权限制（docs §25.8）。
    /// 调用方可显式指定 kind，不能仅凭握手能力字段推断主机已获得配对授权。
    std::string pairing_kind = "upgradeNonAutomationLockdownPairing";
};

/// 在已连接的控制面执行配对，不负责建立连接或写入记录。成功时安装双向主密钥，
/// 失败时 ok=false，error 与 err 记录原因；
/// 已执行的设备端配对步骤不会由本函数回滚，channel 也不保证恢复到初始状态。
/// 已有设备在 setup 后关闭连接，后续起隧道需重连并 verify（验证范围见 docs §25）。
/// 验证 SRP 服务端证明，并要求 M6 身份密文通过 AEAD 和设备 Ed25519 签名校验。
/// 只有这些检查完成后才安装主密钥、发布含设备长期身份的记录。
/// 首次配对的身份信任仍依赖可信入口及用户确认；此处没有 Apple 根证书认证。
PairSetupResult pair_setup(
    Rppairing& channel,
    std::string_view host_identifier,
    std::string_view hostname,
    std::string_view udid,
    const ProgressFn& progress,
    const PairSetupOptions& options,
    std::string& err);

} // namespace maa::ios::wifi
