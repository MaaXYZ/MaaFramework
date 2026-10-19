#pragma once

#include <optional>
#include <string>

#include "wifi/Crypto.h"
#include "wifi/PairRecord.h"
#include "wifi/Rppairing.h"
#include "json/Json.h"

namespace maa::ios::wifi
{

/// Paired 表示流程完成并安装主密钥；NotPaired 表示本地记录不完整或设备返回
/// 配对错误。TransportFailure 是其余失败路径的默认值，也包括消息格式及密码学
/// 操作失败，不能仅凭此值判断网络故障或保证重试会成功。
enum class VerifyOutcome
{
    Paired,
    NotPaired,
    TransportFailure
};

struct PairVerifyResult
{
    VerifyOutcome outcome = VerifyOutcome::TransportFailure;
    /// X25519 共享秘密，用于主密钥派生及后续 TLS-PSK 隧道；应按敏感数据处理。
    /// 仅在身份校验与 M4 均成功后填入，调用方仍应先检查 outcome。
    Bytes shared_secret;
    /// 设备 handshake 响应体，包含 wireProtocolVersion、deviceOptions 等信息。
    json::Value device_handshake;
    std::string error;
};

/// 在已连接的控制面执行 handshake + pair-verify，成功后安装双向主密钥。
/// 主机 Ed25519 签名输入按固定顺序拼接：本次主机 X25519 公钥、记录中的
/// host_identifier、设备 X25519 公钥；标识必须与 setup 注册时一致。
/// 发送主机签名前解密 PV-Msg02，并用配对时保存的设备长期公钥校验签名，
/// 要求设备标识与记录一致。记录缺少设备身份时应重新配对，
/// 不能在网络重连时接受回复中的新公钥。失败时不保证 channel 可直接用于下一次握手。
/// announce_failure 控制设备返回 Error TLV 后是否尽力发送 pairVerifyFailed；
/// 发送失败不覆盖原配对错误。产品默认开启，实验入口可关闭，以研究同连接后续流程。
/// 设备可能在收到失败通知后关闭连接。
PairVerifyResult pair_verify(Rppairing& channel, const PairRecord& host, std::string& err, bool announce_failure = true);

} // namespace maa::ios::wifi
