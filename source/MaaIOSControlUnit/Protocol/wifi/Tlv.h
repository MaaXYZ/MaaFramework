#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "wifi/Crypto.h"

namespace maa::ios::wifi
{

/// 配对数据里的 TLV 类型码。只列本项目会用到的，其余原样透传。
enum class TlvType : uint8_t
{
    Method = 0x00,
    Identifier = 0x01,
    Salt = 0x02,
    PublicKey = 0x03,
    Proof = 0x04,
    EncryptedData = 0x05,
    State = 0x06,
    Error = 0x07,
    Signature = 0x0A,
    Info = 0x11,
};

/// TLV 编码：`type:u8` + `len:u8` + `value`。
///
/// 长度字段只有 8 位，所以超过 255 字节的值必须**拆成多条同类型的 TLV 连续放**，
/// 解码侧再把它们拼回来——这不是可选优化：签名 64 字节看不出问题，但配对建立时
/// 那份 OPACK 设备信息能到 300+ 字节，不拆就是"发出去了但被截断"。
Bytes tlv_build(const std::vector<std::pair<TlvType, Bytes>>& items);

/// 解码并按类型拼接。未知类型照样收进 map（不丢字节，方便把设备的原文打出来）。
std::map<uint8_t, Bytes> tlv_parse(const Bytes& data, std::string& err, bool* truncated = nullptr);

/// 取某个字段的字节；不存在返回 nullptr。
const Bytes* tlv_get(const std::map<uint8_t, Bytes>& fields, TlvType type);

/// 状态字段按惯例是单字节。取不出来时返回 0（调用方拿它判"到没到预期的那一步"）。
uint8_t tlv_state(const std::map<uint8_t, Bytes>& fields);

} // namespace maa::ios::wifi
