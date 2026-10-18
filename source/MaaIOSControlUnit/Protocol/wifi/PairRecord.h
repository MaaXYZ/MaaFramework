#pragma once

#include <string>
#include <vector>

#include "wifi/Crypto.h"

namespace maa::ios::wifi
{

/// 一台设备的远程配对记录。
///
/// host_identifier 保存注册时的主机标识，pair-verify 将同一字符串纳入签名。
/// 主机名变化或迁移记录时应继续使用记录中的值，不能根据当前主机名重新生成。
/// advertised_identifier 保存设备握手中的广播标识；已有样本中它与 USB 的 UDID
/// 不同（docs §22.1）。局域网发现与 lockdown 连接使用不同标识，不能相互替代。
/// 记录包含主机私钥及配对相关密钥，保存位置和访问权限应按敏感数据管理。
struct PairRecord
{
    std::string udid;
    std::string host_identifier;
    std::string peer_identifier;
    Bytes host_private_key; ///< Ed25519 种子，32 字节
    Bytes host_public_key;  ///< Ed25519 公钥，32 字节
    Bytes peer_public_key;  ///< 设备 Ed25519 公钥，32 字节
    Bytes peer_alt_irk;     ///< 设备身份解析密钥，16 字节；未提供时为空
    std::string advertised_identifier;
    std::string remote_unlock_host_key;

    /// 检查主机签名所需标识和密钥长度，不验证公私钥是否匹配或设备端信任状态。
    /// 设备身份是否齐备由 has_peer_identity() 单独检查。
    [[nodiscard]] bool complete() const
    {
        return !udid.empty() && !host_identifier.empty() && host_private_key.size() == 32 && host_public_key.size() == 32;
    }

    [[nodiscard]] bool has_peer_identity() const { return !peer_identifier.empty() && peer_public_key.size() == 32; }
};

/// 单行格式：MaaIOS1|udid=...|hid=...|pid=...|hsk=...|hpk=...|ppk=...|irk=...
/// 密钥使用十六进制；文本中的 % 和 | 分别编码为 %25 和 %7C。
/// parse_record 校验字段及密钥长度，调用方检查 complete() 和 has_peer_identity()。
/// format_record 不校验输入记录。
std::string format_record(const PairRecord& record);
std::optional<PairRecord> parse_record(std::string_view text, std::string& err);

} // namespace maa::ios::wifi
