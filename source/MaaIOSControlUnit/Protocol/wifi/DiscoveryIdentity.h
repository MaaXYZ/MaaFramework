#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "wifi/PairRecord.h"

namespace maa::ios::wifi
{

/// OpenSSL 的 SipHash-2-4，返回 64 位结果的小端字节。key 必须为 16 字节；
/// message 保留完整字节（包括 NUL），可以为空。失败时返回 nullopt 并填写 err。
/// 此接口单独公开，方便用算法发布者的向量验证输出长度和字节顺序。
std::optional<std::array<uint8_t, 8>> siphash24(std::string_view key, std::string_view message, std::string& err);

enum class DiscoveryIdentityStatus
{
    unmatched,
    matched,
    ambiguous,
    invalid_advertisement,
    crypto_error,
};

struct DiscoveryIdentityMatch
{
    DiscoveryIdentityStatus status = DiscoveryIdentityStatus::unmatched;
    /// 仅唯一 authTag 匹配时有值。索引对应传入 records；调用方保留文件路径映射。
    std::optional<size_t> record_index;
    std::vector<size_t> auth_tag_matches;
    /// advertised_identifier 直接相等的记录，仅供展示旧记录线索，不能据此选择
    /// 配对密钥。identifier 可轮换，也可被局域网其他主机伪造。
    std::vector<size_t> identifier_hints;
    std::string error;
};

/// 匹配 _remotepairing._tcp 的 TXT identifier/authTag 与本地配对记录。
///
/// identifier 是原始文本字节，不能转换为 UUID 二进制、改变大小写或去除空白。
/// 接受非空、最多 255 字节、无 ASCII 空白/控制字符的标识，兼容非 UUID 标识。
/// authTag 若存在，必须为 8 个标准 Base64 字符，解码后正好 6 字节；不接受
/// hex、URL-safe Base64、padding 或空白。空 tag 可提供 identifier_hints，
/// 但不会选出记录。无 16 字节 altIRK 的旧记录只能作为 identifier_hints。
///
/// 标签来自 SipHash-2-4 的 8 字节小端输出，取前 6 字节再反转。其作用是发现
/// 候选设备；48 位标签不能替代设备认证。匹配成功也不代表 record.complete()
/// 或 has_peer_identity()，连接前仍须检查记录并执行 PairVerify。多个记录匹配
/// 时返回 ambiguous，包含所有索引，不任取第一条。
DiscoveryIdentityMatch match_advertisement(std::string_view identifier, std::string_view auth_tag, const std::vector<PairRecord>& records);

} // namespace maa::ios::wifi
