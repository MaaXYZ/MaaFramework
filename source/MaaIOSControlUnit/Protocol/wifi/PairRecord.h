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
    Bytes host_private_key; ///< Ed25519 种子，32 字节
    Bytes host_public_key;  ///< Ed25519 公钥，32 字节
    std::string advertised_identifier;
    /// 从经过签名校验的 PairSetup M6 保存的设备长期身份。identifier 保留协议原始
    /// 字节，不能用 USB UDID、广播标识或当前主机标识替代。
    Bytes peer_identifier;
    Bytes peer_public_key; ///< 设备 Ed25519 公钥，32 字节；旧记录可以缺失
    Bytes peer_alt_irk;    ///< 设备的身份解析密钥，16 字节；未提供时可空，兼容旧记录
    std::string remote_unlock_host_key;

    /// 检查主机签名所需标识和密钥长度，不验证公私钥是否匹配或设备端信任状态。
    /// 设备身份是否齐备由 has_peer_identity() 单独检查。
    [[nodiscard]] bool complete() const
    {
        return !udid.empty() && !host_identifier.empty() && host_private_key.size() == 32 && host_public_key.size() == 32;
    }

    /// 旧记录仍可读取以定位设备，但只有保存了长期身份的记录才能认证 PairVerify M2。
    [[nodiscard]] bool has_peer_identity() const { return !peer_identifier.empty() && peer_public_key.size() == 32; }
};

/// 版本 1 文本格式：首个非空行为固定头，后续为 key=value，二进制字段使用十六进制。
/// parse_record 检查已知二进制字段长度、拒绝已知键重复并要求私钥存在，忽略未知键。
/// peer_identifier 存在时不能为空，peer_public_key 存在时必须为 32 字节；旧记录
/// 可以缺失这些字段。成功解析不等于 complete() 或 has_peer_identity()，调用方
/// 仍需检查用途所需的字段。format_record 不校验输入记录。
std::string format_record(const PairRecord& record);
std::optional<PairRecord> parse_record(std::string_view text, std::string& err);

/// 同目录临时文件写入后替换目标。POSIX 文件在写入内容前设为 0600，新建叶子目录
/// 设为 0700；Windows 为文件和新建叶子目录设置当前进程用户 SID 的受保护 DACL。
/// 已有目录和上级目录权限不在此调整。失败返回 false 并填 err，尽量移除临时文件。
bool save_record(const std::string& path, const PairRecord& record, std::string& err);
/// Same private temporary-file write, but atomically refuses an existing target
/// (including symlinks). Used for wireless first pairing; never replaces trust.
bool save_record_new(const std::string& path, const PairRecord& record, std::string& err);
/// 最多读取 16 KiB 加一个检测字节，拒绝超限或读取失败，然后解析完整文本。
std::optional<PairRecord> load_record(const std::string& path, std::string& err);

/// 从目录的普通文件名中提取 remote- 与 .pair 之间的候选标识并排序，不解析文件内容。
/// 返回值是文件名中的候选标识，不能据此还原原始 UDID 的标点。
/// 目录未被识别为目录时返回空列表并清除 err，包括不存在或目录查询失败的情况。
/// 枚举过程中检测到的错误通过 err 返回；列表可用于 Wi-Fi 连接选择本地记录。
std::vector<std::string> list_record_udids(const std::string& dir, std::string& err);

} // namespace maa::ios::wifi
