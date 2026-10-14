#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "wifi/Crypto.h"

namespace maa::ios::wifi
{

/// 当前配对流程使用的 OPACK 对象适配。pair-setup 的 M5/M6 通过 TLV INFO
/// 传递设备信息字典，例如 altIRK、mac、name。
/// 支持 bool、整数、字符串、字节串、数组和字典；并非完整 OPACK 类型集合。
///
/// bool 使用 0x01/0x02，0～39 的整数直接编码在 0x08～0x2F 中；整数载荷
/// 使用小端，0x30/0x32/0x33 分别带 1/4/8 字节。编码器只接受非负整数。
/// 字符串的短长度标记是 0x40～0x60，字节串是 0x70～0x90；更长载荷的
/// 长度字段使用大端，字符串 0x61～0x64、字节串 0x91～0x94 分别带 1/2/4/8 字节。
/// 编码器最长写出 4 字节长度，解码器也接受 8 字节长度形态。
/// 数组 0xD0～0xDE、字典 0xE0～0xEE 在标记中声明项数；15 项及以上编码为
/// 0xDF/0xEF 的终止符形态，数组结尾写一个 0x03，字典结尾写两个。
/// tests/wifi_test.cpp 包含参考编码字节的离线兼容性用例。
struct OpackValue
{
    enum class Kind
    {
        kBool,
        kInt,
        kBytes,
        kString,
        kList,
        kDict
    } kind = Kind::kBool;
    bool boolean = false;
    int64_t integer = 0;
    Bytes bytes;
    std::string str;
    std::vector<OpackValue> list;
    std::vector<std::pair<OpackValue, OpackValue>> dict; ///< 保留插入顺序，编码时沿用该顺序

    static OpackValue of_bytes(Bytes b)
    {
        OpackValue v;
        v.kind = Kind::kBytes;
        v.bytes = std::move(b);
        return v;
    }

    static OpackValue of_string(std::string s)
    {
        OpackValue v;
        v.kind = Kind::kString;
        v.str = std::move(s);
        return v;
    }

    /// 按字符串键找字典值。找不到返回 nullptr。
    [[nodiscard]] const OpackValue* find(std::string_view key) const;
};

/// 将编码追加到 out；失败可能留下部分输出，调用方负责清空或丢弃。
bool opack_encode(const OpackValue& value, Bytes& out, std::string& err);
/// 解码一个完整对象，拒绝尾随字节；递归层级从 0 计，超过 16 时失败。
bool opack_decode(const Bytes& in, OpackValue& out, std::string& err);

} // namespace maa::ios::wifi
