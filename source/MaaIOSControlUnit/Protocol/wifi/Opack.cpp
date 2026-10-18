#include "wifi/Opack.h"

#include <cstdio>

namespace maa::ios::wifi
{
namespace
{

// 不定长数组以一个终止符结束，字典编码以两个终止符结束。
constexpr uint8_t kTerminator = 0x03;

void put_u8(Bytes& out, uint8_t v)
{
    out.push_back(v);
}

void put_u16_be(Bytes& out, uint16_t v)
{
    out.push_back(static_cast<uint8_t>(v >> 8));
    out.push_back(static_cast<uint8_t>(v));
}

void put_u32_be(Bytes& out, uint32_t v)
{
    put_u16_be(out, static_cast<uint16_t>(v >> 16));
    put_u16_be(out, static_cast<uint16_t>(v));
}

void put_le(Bytes& out, uint64_t v, int width)
{
    for (int i = 0; i < width; ++i) {
        out.push_back(static_cast<uint8_t>(v >> (8 * i)));
    }
}

bool read_u8(const Bytes& in, size_t& pos, uint8_t& v)
{
    if (pos >= in.size()) {
        return false;
    }
    v = in[pos++];
    return true;
}

bool read_be(const Bytes& in, size_t& pos, int width, uint64_t& v)
{
    if (pos > in.size() || static_cast<size_t>(width) > in.size() - pos) {
        return false;
    }
    v = 0;
    for (int i = 0; i < width; ++i) {
        v = (v << 8) | in[pos++];
    }
    return true;
}

bool read_le(const Bytes& in, size_t& pos, int width, uint64_t& v)
{
    if (pos > in.size() || static_cast<size_t>(width) > in.size() - pos) {
        return false;
    }
    v = 0;
    for (int i = width - 1; i >= 0; --i) {
        v = (v << 8) | in[pos + static_cast<size_t>(i)];
    }
    pos += static_cast<size_t>(width);
    return true;
}

bool encode_one(const OpackValue& v, Bytes& out, std::string& err);

bool encode_lengthed(Bytes& out, uint8_t short_base, uint8_t l1, uint8_t l2, uint8_t l4, size_t len, const Bytes& payload)
{
    if (len <= 0x20) {
        put_u8(out, static_cast<uint8_t>(short_base + len));
    }
    else if (len <= 0xFF) {
        put_u8(out, l1);
        put_u8(out, static_cast<uint8_t>(len));
    }
    else if (len <= 0xFFFF) {
        put_u8(out, l2);
        put_u16_be(out, static_cast<uint16_t>(len));
    }
    else if (len < 0x100000000ULL) {
        put_u8(out, l4);
        put_u32_be(out, static_cast<uint32_t>(len));
    }
    else {
        return false;
    }
    out.insert(out.end(), payload.begin(), payload.end());
    return true;
}

bool encode_one(const OpackValue& v, Bytes& out, std::string& err)
{
    switch (v.kind) {
    case OpackValue::Kind::kBool:
        put_u8(out, v.boolean ? 0x01 : 0x02);
        return true;
    case OpackValue::Kind::kInt: {
        const int64_t n = v.integer;
        if (n < 0) {
            err = "OPACK encoder does not support negative integers";
            return false;
        }
        const auto u = static_cast<uint64_t>(n);
        if (u <= 0x27) {
            put_u8(out, static_cast<uint8_t>(8 + u));
        }
        else if (u <= 0xFF) {
            put_u8(out, 0x30);
            put_le(out, u, 1);
        }
        else if (u <= 0xFFFFFFFFULL) {
            put_u8(out, 0x32);
            put_le(out, u, 4);
        }
        else {
            put_u8(out, 0x33);
            put_le(out, u, 8);
        }
        return true;
    }
    case OpackValue::Kind::kString: {
        const Bytes payload(v.str.begin(), v.str.end());
        return encode_lengthed(out, 0x40, 0x61, 0x62, 0x63, payload.size(), payload);
    }
    case OpackValue::Kind::kBytes:
        return encode_lengthed(out, 0x70, 0x91, 0x92, 0x93, v.bytes.size(), v.bytes);
    case OpackValue::Kind::kList: {
        if (v.list.size() < 15) {
            put_u8(out, static_cast<uint8_t>(0xD0 + v.list.size()));
            for (const auto& e : v.list) {
                if (!encode_one(e, out, err)) {
                    return false;
                }
            }
            return true;
        }
        put_u8(out, 0xDF);
        for (const auto& e : v.list) {
            if (!encode_one(e, out, err)) {
                return false;
            }
        }
        put_u8(out, kTerminator);
        return true;
    }
    case OpackValue::Kind::kDict: {
        if (v.dict.size() < 15) {
            put_u8(out, static_cast<uint8_t>(0xE0 + v.dict.size()));
        }
        else {
            put_u8(out, 0xEF);
        }
        for (const auto& [k, val] : v.dict) {
            if (!encode_one(k, out, err) || !encode_one(val, out, err)) {
                return false;
            }
        }
        if (v.dict.size() >= 15) {
            put_u8(out, kTerminator);
            put_u8(out, kTerminator);
        }
        return true;
    }
    }
    err = "OPACK encode: unknown value type";
    return false;
}

bool decode_one(const Bytes& in, size_t& pos, OpackValue& out, std::string& err, int depth);

// 字符串与字节串的长度字段使用大端，短形态直接从类型标记取长度。
bool decode_payload(const Bytes& in, size_t& pos, uint8_t type, Bytes& payload, std::string& err)
{
    uint64_t len = 0;
    if (type == 0x61 || type == 0x91) {
        uint8_t l = 0;
        if (!read_u8(in, pos, l)) {
            err = "OPACK decode: payload exceeds buffer";
            return false;
        }
        len = l;
    }
    else if (type == 0x62 || type == 0x92) {
        if (!read_be(in, pos, 2, len)) {
            err = "OPACK decode: payload exceeds buffer";
            return false;
        }
    }
    else if (type == 0x63 || type == 0x93) {
        if (!read_be(in, pos, 4, len)) {
            err = "OPACK decode: payload exceeds buffer";
            return false;
        }
    }
    else if (type == 0x64 || type == 0x94) {
        if (!read_be(in, pos, 8, len)) {
            err = "OPACK decode: payload exceeds buffer";
            return false;
        }
    }
    else if (type >= 0x40 && type <= 0x60) {
        len = type - 0x40;
    }
    else if (type >= 0x70 && type <= 0x90) {
        len = type - 0x70;
    }
    else {
        err = "OPACK decode: unknown length form";
        return false;
    }
    // 先比较剩余长度，避免不可信的 64 位声明长度与 pos 相加回绕。
    if (pos > in.size() || len > in.size() - pos) {
        err = "OPACK decode: payload exceeds buffer";
        return false;
    }
    const auto end = pos + static_cast<size_t>(len); // 上述检查保证 end 位于输入内。
    // 使用迭代器自身的差值类型，避免 Windows 的 32 位 long 截断位置。
    payload.assign(in.begin() + static_cast<Bytes::difference_type>(pos), in.begin() + static_cast<Bytes::difference_type>(end));
    pos = end;
    return true;
}

bool decode_one(const Bytes& in, size_t& pos, OpackValue& out, std::string& err, int depth)
{
    // 当前适配的递归输入限制，层级从根对象的 0 开始。
    if (depth > 16) {
        err = "OPACK decode: nesting too deep";
        return false;
    }
    uint8_t type = 0;
    if (!read_u8(in, pos, type)) {
        err = "OPACK decode: missing type byte";
        return false;
    }
    if (type == kTerminator) {
        err = "OPACK decode: terminator in value position";
        return false;
    }
    if (type == 0x01 || type == 0x02) {
        out = OpackValue { };
        out.kind = OpackValue::Kind::kBool;
        out.boolean = type == 0x01;
        return true;
    }
    if (type == 0x06) { // 时间戳形态：读取并跳过 8 字节小端载荷，不生成时间戳值
        uint64_t skip = 0;
        return read_le(in, pos, 8, skip);
    }
    if (type >= 0x08 && type <= 0x2F) {
        out = OpackValue { };
        out.kind = OpackValue::Kind::kInt;
        out.integer = type - 8;
        return true;
    }
    if (type == 0x30 || type == 0x32 || type == 0x33) {
        const int width = type == 0x30 ? 1 : (type == 0x32 ? 4 : 8);
        uint64_t v = 0;
        if (!read_le(in, pos, width, v)) {
            return false;
        }
        out = OpackValue { };
        out.kind = OpackValue::Kind::kInt;
        out.integer = static_cast<int64_t>(v);
        return true;
    }
    if ((type >= 0x40 && type <= 0x64) || (type >= 0x70 && type <= 0x94)) {
        Bytes payload;
        if (!decode_payload(in, pos, type, payload, err)) {
            return false;
        }
        out = OpackValue { };
        if (type <= 0x64) {
            out.kind = OpackValue::Kind::kString;
            out.str.assign(payload.begin(), payload.end());
        }
        else {
            out.kind = OpackValue::Kind::kBytes;
            out.bytes = std::move(payload);
        }
        return true;
    }
    if (type >= 0xD0 && type <= 0xDF) {
        out = OpackValue { };
        out.kind = OpackValue::Kind::kList;
        const bool terminated = type == 0xDF;
        const size_t n = terminated ? SIZE_MAX : type - 0xD0;
        for (size_t i = 0; i < n; ++i) {
            if (pos < in.size() && in[pos] == kTerminator) {
                if (!terminated) {
                    err = "OPACK decode: terminator in fixed-length array";
                    return false;
                }
                ++pos;
                return true;
            }
            OpackValue e;
            if (!decode_one(in, pos, e, err, depth + 1)) {
                return false;
            }
            out.list.push_back(std::move(e));
        }
        return true;
    }
    if (type >= 0xE0 && type <= 0xEF) {
        out = OpackValue { };
        out.kind = OpackValue::Kind::kDict;
        const bool terminated = type == 0xEF;
        const size_t n = terminated ? SIZE_MAX : type - 0xE0;
        for (size_t i = 0; i < n; ++i) {
            if (pos < in.size() && in[pos] == kTerminator) {
                if (!terminated) {
                    err = "OPACK decode: terminator in fixed-length dictionary";
                    return false;
                }
                pos += 2; // 当前适配按键和值各一个终止符推进
                return true;
            }
            OpackValue k;
            OpackValue v;
            if (!decode_one(in, pos, k, err, depth + 1) || !decode_one(in, pos, v, err, depth + 1)) {
                return false;
            }
            out.dict.emplace_back(std::move(k), std::move(v));
        }
        return true;
    }
    err = "OPACK decode: unknown type byte 0x" + [type] {
        char buf[3];
        std::snprintf(buf, sizeof(buf), "%02x", type);
        return std::string(buf);
    }();
    return false;
}

} // namespace

const OpackValue* OpackValue::find(const std::string_view key) const
{
    if (kind != Kind::kDict) {
        return nullptr;
    }
    for (const auto& [k, v] : dict) {
        if (k.kind == Kind::kString && k.str == key) {
            return &v;
        }
    }
    return nullptr;
}

bool opack_encode(const OpackValue& value, Bytes& out, std::string& err)
{
    return encode_one(value, out, err);
}

bool opack_decode(const Bytes& in, OpackValue& out, std::string& err)
{
    size_t pos = 0;
    if (!decode_one(in, pos, out, err, 0)) {
        return false;
    }
    if (pos != in.size()) {
        err = "OPACK decode: " + std::to_string(in.size() - pos) + " trailing bytes";
        return false;
    }
    return true;
}

} // namespace maa::ios::wifi
