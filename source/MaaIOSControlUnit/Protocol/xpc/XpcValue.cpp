#include "xpc/XpcValue.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>

namespace maa::ios::xpc
{
namespace
{

/// 编解码的递归层级从 0 计，超过 64 时拒绝继续处理。
constexpr int kMaxDepth = 64;
/// 字符串和数据的长度字段沿用头文件中的共享输入上限。
/// 32 MiB 是当前应用的资源策略，可容纳已验证的多 MiB 内联截图；
/// 具体读边界仍由 Reader::take() 按剩余字节检查。
constexpr uint32_t kMaxLen = static_cast<uint32_t>(kMaxBuffer);

std::string u64_to_string(uint64_t v)
{
    if (v == 0) {
        return "0";
    }
    std::string s;
    while (v) {
        s.insert(s.begin(), static_cast<char>('0' + v % 10));
        v /= 10;
    }
    return s;
}

std::string hex(uint32_t v)
{
    char buf[11];
    std::snprintf(buf, sizeof(buf), "%08x", v);
    return buf;
}

std::string i64_to_string(int64_t v)
{
    // 用无符号运算取绝对值，避免对 INT64_MIN 直接取负造成溢出。
    const bool neg = v < 0;
    const uint64_t mag = neg ? (~static_cast<uint64_t>(v) + 1) : static_cast<uint64_t>(v);
    std::string s = u64_to_string(mag);
    if (neg) {
        s.insert(s.begin(), '-');
    }
    return s;
}

// ------------------------------------------------------------------ 写入 ------

void put_u32(std::vector<uint8_t>& out, uint32_t v)
{
    out.push_back(static_cast<uint8_t>(v));
    out.push_back(static_cast<uint8_t>(v >> 8));
    out.push_back(static_cast<uint8_t>(v >> 16));
    out.push_back(static_cast<uint8_t>(v >> 24));
}

void put_u64(std::vector<uint8_t>& out, uint64_t v)
{
    for (int i = 0; i < 8; ++i) {
        out.push_back(static_cast<uint8_t>(v >> (8 * i)));
    }
}

/// 相对本段起点 start 补零到 4 字节边界；嵌套段分别计算对齐。
void align4(std::vector<uint8_t>& out, std::size_t start)
{
    const std::size_t used = out.size() - start;
    out.insert(out.end(), (4 - used % 4) % 4, 0);
}

/// 字典键编码为 NUL 结尾字符串并补齐到 4 字节边界，不带长度前缀。
void put_cstr(std::vector<uint8_t>& out, const std::string& s)
{
    const std::size_t start = out.size();
    out.insert(out.end(), s.begin(), s.end());
    out.push_back(0);
    align4(out, start);
}

bool encode_into(std::vector<uint8_t>& out, const Value& v, int depth)
{
    if (depth > kMaxDepth) {
        return false;
    }
    put_u32(out, static_cast<uint32_t>(v.type));
    switch (v.type) {
    case Type::Null:
        return true;
    case Type::Bool:
        put_u32(out, v.boolean ? 1 : 0);
        return true;
    case Type::Int64:
        put_u64(out, static_cast<uint64_t>(v.int64));
        return true;
    case Type::UInt64:
    case Type::Date:
        put_u64(out, v.uint64);
        return true;
    case Type::Double: {
        uint64_t bits = 0;
        std::memcpy(&bits, &v.real, sizeof(bits));
        put_u64(out, bits);
        return true;
    }
    case Type::String: {
        // 字符串长度包含末尾 NUL；Data 的长度只包含实际字节。
        const std::size_t start = out.size();
        put_u32(out, static_cast<uint32_t>(v.string.size() + 1));
        out.insert(out.end(), v.string.begin(), v.string.end());
        out.push_back(0);
        align4(out, start);
        return true;
    }
    case Type::Data: {
        const std::size_t start = out.size();
        put_u32(out, static_cast<uint32_t>(v.data.size()));
        out.insert(out.end(), v.data.begin(), v.data.end());
        align4(out, start);
        return true;
    }
    case Type::Uuid:
        if (v.data.size() != 16) {
            return false;
        }
        out.insert(out.end(), v.data.begin(), v.data.end());
        return true;
    case Type::FileTransfer: {
        // 线上形态：u64 传输号 + 一个内层字典，字典里的 "s" 是字节数。
        put_u64(out, v.transfer_id);
        auto meta = make_dict();
        dict_set(meta, "s", make_uint64(v.file_size));
        return encode_into(out, meta, depth + 1);
    }
    case Type::Array:
    case Type::Dict: {
        // 容器长度包含 count 字段和全部条目，不含类型标记及长度字段本身。
        // 先编码内容以确定该长度，再写入外层缓冲区。
        std::vector<uint8_t> content;
        put_u32(content, static_cast<uint32_t>(v.type == Type::Array ? v.array.size() : v.dict.size()));
        if (v.type == Type::Array) {
            for (const auto& item : v.array) {
                if (!encode_into(content, item, depth + 1)) {
                    return false;
                }
            }
        }
        else {
            for (const auto& entry : v.dict) {
                put_cstr(content, entry.key);
                if (!encode_into(content, entry.value, depth + 1)) {
                    return false;
                }
            }
        }
        put_u32(out, static_cast<uint32_t>(content.size()));
        out.insert(out.end(), content.begin(), content.end());
        return true;
    }
    }
    return false;
}

// ------------------------------------------------------------------ 读取 ------

class Reader
{
public:
    Reader(const uint8_t* begin, const uint8_t* end)
        : p_(begin)
        , end_(end)
    {
    }

    [[nodiscard]] std::size_t remaining() const { return static_cast<std::size_t>(end_ - p_); }

    [[nodiscard]] const uint8_t* here() const { return p_; }

    [[nodiscard]] const std::string& err() const { return err_; }

    void set_err(const std::string& what)
    {
        if (err_.empty()) {
            err_ = what;
        }
    }

    bool u32(uint32_t& v)
    {
        if (remaining() < 4) {
            return fail("u32 read out of bounds");
        }
        v = static_cast<uint32_t>(p_[0]) | static_cast<uint32_t>(p_[1]) << 8 | static_cast<uint32_t>(p_[2]) << 16
            | static_cast<uint32_t>(p_[3]) << 24;
        p_ += 4;
        return true;
    }

    bool u64(uint64_t& v)
    {
        if (remaining() < 8) {
            return fail("u64 read out of bounds");
        }
        v = 0;
        for (int i = 0; i < 8; ++i) {
            v |= static_cast<uint64_t>(p_[i]) << (8 * i);
        }
        p_ += 8;
        return true;
    }

    bool take(std::size_t n, std::vector<uint8_t>& out)
    {
        if (remaining() < n) {
            return fail("Fixed-length byte read out of bounds");
        }
        out.assign(p_, p_ + n);
        p_ += n;
        return true;
    }

    /// 按相对 start 的 4 字节对齐跳过填充，最多前进到当前段末。
    /// 填充不足时停在段末，避免游标越界以及 remaining() 的无符号下溢。
    void align_from(const uint8_t* start)
    {
        const auto used = static_cast<std::size_t>(p_ - start);
        const std::size_t pad = (4 - used % 4) % 4;
        p_ += std::min(pad, remaining());
    }

    /// 在当前段内读取 NUL 结尾的字典键，并跳过对齐填充；段内无 NUL 时失败。
    bool cstr(std::string& out)
    {
        const uint8_t* start = p_;
        const uint8_t* nul = static_cast<const uint8_t*>(std::memchr(p_, 0, remaining()));
        if (nul == nullptr) {
            return fail("Key has no NUL terminator in this segment");
        }
        out.assign(reinterpret_cast<const char*>(p_), static_cast<std::size_t>(nul - p_));
        p_ = nul + 1;
        align_from(start);
        return true;
    }

    /// 将接下来的 n 字节交给子读取器，并把外层游标推进到子段末。
    /// 子段不能超过当前剩余字节；后续同层对象从子段之后继续读取。
    bool enter(std::size_t n, Reader& out)
    {
        if (remaining() < n) {
            return fail("Length prefix exceeds remaining bytes");
        }
        out = Reader(p_, p_ + n);
        p_ += n;
        return true;
    }

    [[nodiscard]] bool ok() const { return err_.empty(); }

private:
    bool fail(const char* what)
    {
        set_err(what);
        return false;
    }

    const uint8_t* p_;
    const uint8_t* end_;
    std::string err_;
};

bool decode_into(Reader& r, Value& out, int depth)
{
    if (depth > kMaxDepth) {
        r.set_err("XPC nesting too deep");
        return false;
    }
    uint32_t tag = 0;
    if (!r.u32(tag)) {
        return false;
    }
    switch (tag) {
    case static_cast<uint32_t>(Type::Null):
        out = make_null();
        return true;
    case static_cast<uint32_t>(Type::Bool): {
        uint32_t raw = 0;
        if (!r.u32(raw)) {
            return false;
        }
        out = make_bool(raw != 0);
        return true;
    }
    case static_cast<uint32_t>(Type::Int64): {
        uint64_t raw = 0;
        if (!r.u64(raw)) {
            return false;
        }
        out = make_int64(static_cast<int64_t>(raw));
        return true;
    }
    case static_cast<uint32_t>(Type::UInt64): {
        uint64_t raw = 0;
        if (!r.u64(raw)) {
            return false;
        }
        out = make_uint64(raw);
        return true;
    }
    case static_cast<uint32_t>(Type::Date): {
        uint64_t raw = 0;
        if (!r.u64(raw)) {
            return false;
        }
        out = make_date(raw);
        return true;
    }
    case static_cast<uint32_t>(Type::Double): {
        uint64_t bits = 0;
        if (!r.u64(bits)) {
            return false;
        }
        double d = 0;
        std::memcpy(&d, &bits, sizeof(d));
        out = make_double(d);
        return true;
    }
    case static_cast<uint32_t>(Type::String): {
        const uint8_t* start = r.here();
        uint32_t len = 0;
        if (!r.u32(len)) {
            return false;
        }
        if (len == 0 || len > kMaxLen) {
            r.set_err("Invalid string length");
            return false;
        }
        std::vector<uint8_t> bytes;
        if (!r.take(len, bytes)) {
            return false;
        }
        r.align_from(start);
        // 长度含终结 NUL；按 C 字符串的语义取到第一个 NUL 为止。
        const auto nul = std::find(bytes.begin(), bytes.end(), uint8_t { 0 });
        out = make_string(std::string(bytes.begin(), nul == bytes.end() ? bytes.end() : nul));
        return true;
    }
    case static_cast<uint32_t>(Type::Data): {
        const uint8_t* start = r.here();
        uint32_t len = 0;
        if (!r.u32(len)) {
            return false;
        }
        if (len > kMaxLen) {
            // 错误同时记录声明长度与当前限制，便于判断超限幅度。
            r.set_err("Invalid data length: declared " + std::to_string(len) + " bytes, limit " + std::to_string(kMaxLen));
            return false;
        }
        Value v;
        v.type = Type::Data;
        if (!r.take(len, v.data)) {
            return false;
        }
        r.align_from(start);
        out = std::move(v);
        return true;
    }
    case static_cast<uint32_t>(Type::Uuid): {
        Value v;
        v.type = Type::Uuid;
        if (!r.take(16, v.data)) {
            return false;
        }
        out = std::move(v);
        return true;
    }
    case static_cast<uint32_t>(Type::FileTransfer): {
        uint64_t msg_id = 0;
        if (!r.u64(msg_id)) {
            return false;
        }
        Value meta;
        if (!decode_into(r, meta, depth + 1)) {
            return false;
        }
        Value v;
        v.type = Type::FileTransfer;
        v.transfer_id = msg_id;
        v.file_size = static_cast<uint64_t>(meta.at("s").as_int_or(0));
        out = std::move(v);
        return true;
    }
    case static_cast<uint32_t>(Type::Array):
    case static_cast<uint32_t>(Type::Dict): {
        uint32_t total = 0;
        if (!r.u32(total)) {
            return false;
        }
        if (total < 4) {
            r.set_err("Container length cannot hold count field");
            return false;
        }
        // 仅在 total 指定的子段内解析条目，不能读取后续同层对象的字节。
        // enter 同时推进外层游标到子段末。
        Reader body(r.here(), r.here());
        if (!r.enter(total, body)) {
            return false;
        }
        uint32_t count = 0;
        if (!body.u32(count)) {
            r.set_err(body.err());
            return false;
        }
        // 数组条目至少 4 字节（Null 的类型标记）；字典条目至少 8 字节
        // （NUL 键补齐到 4 字节，加 4 字节值）。分配条目之前先据此检查 count。
        const std::size_t min_entry = tag == static_cast<uint32_t>(Type::Array) ? 4 : 8;
        if (static_cast<std::size_t>(count) > body.remaining() / min_entry) {
            body.set_err("Entry count does not match container size");
            r.set_err(body.err());
            return false;
        }
        Value v;
        v.type = tag == static_cast<uint32_t>(Type::Array) ? Type::Array : Type::Dict;
        bool parsed = true;
        if (v.type == Type::Array) {
            v.array.resize(count);
            for (uint32_t i = 0; i < count && parsed; ++i) {
                parsed = decode_into(body, v.array[i], depth + 1);
            }
        }
        else {
            v.dict.resize(count);
            for (uint32_t i = 0; i < count && parsed; ++i) {
                parsed = body.cstr(v.dict[i].key) && decode_into(body, v.dict[i].value, depth + 1);
            }
        }
        if (!parsed) {
            r.set_err(body.err().empty() ? "Container entry decode failed" : body.err());
            return false;
        }
        out = std::move(v);
        return true;
    }
    default:
        r.set_err("Unsupported XPC type marker 0x" + hex(tag));
        return false;
    }
}

} // namespace

// ---------------------------------------------------------------- Value ------

const Value* Value::find(std::string_view key) const
{
    for (const auto& entry : dict) {
        if (entry.key == key) {
            return &entry.value;
        }
    }
    return nullptr;
}

const Value& Value::at(std::string_view key) const
{
    static const Value k_missing; // Type::Null，所有取值器都会回落到默认值
    const auto* v = find(key);
    return v != nullptr ? *v : k_missing;
}

std::string Value::as_string_or(std::string_view fallback) const
{
    return type == Type::String ? string : std::string(fallback);
}

int64_t Value::as_int_or(int64_t fallback) const
{
    switch (type) {
    case Type::Int64:
        return int64;
    case Type::UInt64:
        return static_cast<int64_t>(uint64);
    case Type::Bool:
        return boolean ? 1 : 0;
    default:
        return fallback;
    }
}

bool Value::as_bool_or(bool fallback) const
{
    return type == Type::Bool ? boolean : fallback;
}

Value make_null()
{
    return Value { };
}

Value make_bool(bool v)
{
    Value x;
    x.type = Type::Bool;
    x.boolean = v;
    return x;
}

Value make_int64(int64_t v)
{
    Value x;
    x.type = Type::Int64;
    x.int64 = v;
    return x;
}

Value make_uint64(uint64_t v)
{
    Value x;
    x.type = Type::UInt64;
    x.uint64 = v;
    return x;
}

Value make_double(double v)
{
    Value x;
    x.type = Type::Double;
    x.real = v;
    return x;
}

Value make_date(uint64_t ns)
{
    Value x;
    x.type = Type::Date;
    x.uint64 = ns;
    return x;
}

Value make_string(std::string v)
{
    Value x;
    x.type = Type::String;
    x.string = std::move(v);
    return x;
}

Value make_data(std::vector<uint8_t> v)
{
    Value x;
    x.type = Type::Data;
    x.data = std::move(v);
    return x;
}

Value make_uuid(std::span<const uint8_t> v)
{
    Value x;
    x.type = Type::Uuid;
    x.data.assign(v.begin(), v.end());
    return x;
}

Value make_file_transfer(uint64_t size, uint64_t transfer_id)
{
    Value x;
    x.type = Type::FileTransfer;
    x.file_size = size;
    x.transfer_id = transfer_id;
    return x;
}

Value make_array()
{
    Value x;
    x.type = Type::Array;
    return x;
}

Value make_dict()
{
    Value x;
    x.type = Type::Dict;
    return x;
}

void array_push(Value& arr, Value item)
{
    if (arr.is_array()) {
        arr.array.push_back(std::move(item));
    }
}

void dict_set(Value& dict, std::string key, Value item)
{
    if (!dict.is_dict()) {
        return;
    }
    for (auto& entry : dict.dict) {
        if (entry.key == key) {
            entry.value = std::move(item);
            return;
        }
    }
    dict.dict.push_back(Entry { std::move(key), std::move(item) });
}

// ------------------------------------------------------------ 对象编解码 ------

std::vector<uint8_t> encode(const Value& v)
{
    std::vector<uint8_t> out;
    if (!encode_into(out, v, 0)) {
        return { };
    }
    return out;
}

std::optional<Value> decode(std::span<const uint8_t> buf, std::string& err)
{
    if (buf.size() > kMaxBuffer) {
        err = "Buffer too large";
        return std::nullopt;
    }
    Reader r(buf.data(), buf.data() + buf.size());
    Value v;
    if (!decode_into(r, v, 0)) {
        err = r.err().empty() ? "Decode failed" : r.err();
        return std::nullopt;
    }
    return v;
}

// ---------------------------------------------------------- 消息（wrapper） ---

std::vector<uint8_t> encode_message(uint32_t flags, uint64_t message_id, const Value* body)
{
    std::vector<uint8_t> payload;
    if (body != nullptr) {
        const auto obj = encode(*body);
        if (obj.empty()) {
            return { };
        }
        put_u32(payload, kPayloadMagic);
        put_u32(payload, kProtocolVersion);
        payload.insert(payload.end(), obj.begin(), obj.end());
    }
    std::vector<uint8_t> out;
    put_u32(out, kWrapperMagic);
    put_u32(out, flags);
    // 长度只包含载荷，不含信封中的 8 字节 message_id。
    put_u64(out, static_cast<uint64_t>(payload.size()));
    put_u64(out, message_id);
    out.insert(out.end(), payload.begin(), payload.end());
    return out;
}

Status decode_message(std::span<const uint8_t> buf, Message& out, std::size_t& consumed, std::string& err)
{
    consumed = 0;
    if (buf.size() < 24) {
        err = "Envelope shorter than 24 bytes";
        return Status::NeedMore;
    }
    const uint32_t magic = static_cast<uint32_t>(buf[0]) | static_cast<uint32_t>(buf[1]) << 8 | static_cast<uint32_t>(buf[2]) << 16
                           | static_cast<uint32_t>(buf[3]) << 24;
    if (magic != kWrapperMagic) {
        err = "Wrapper magic mismatch: 0x" + hex(magic);
        return Status::Malformed;
    }
    uint64_t body_len = 0;
    for (int i = 0; i < 8; ++i) {
        body_len |= static_cast<uint64_t>(buf[8 + i]) << (8 * i);
    }
    if (body_len > kMaxBuffer) {
        err = "Invalid message length: " + u64_to_string(body_len);
        return Status::Malformed;
    }
    const std::size_t total = 24 + static_cast<std::size_t>(body_len);
    if (buf.size() < total) {
        err = "Missing " + u64_to_string(total - buf.size()) + " bytes";
        return Status::NeedMore;
    }

    Message m;
    m.flags = static_cast<uint32_t>(buf[4]) | static_cast<uint32_t>(buf[5]) << 8 | static_cast<uint32_t>(buf[6]) << 16
              | static_cast<uint32_t>(buf[7]) << 24;
    for (int i = 0; i < 8; ++i) {
        m.message_id |= static_cast<uint64_t>(buf[16 + i]) << (8 * i);
    }
    if (body_len > 0) {
        Reader r(buf.data() + 24, buf.data() + total);
        uint32_t pmagic = 0;
        if (!r.u32(pmagic) || pmagic != kPayloadMagic) {
            err = "Payload magic mismatch: 0x" + hex(pmagic);
            return Status::Malformed;
        }
        uint32_t version = 0;
        if (!r.u32(version)) {
            err = "Cannot read protocol version";
            return Status::Malformed;
        }
        if (version != kProtocolVersion) {
            err = "Unsupported XPC protocol version: 0x" + hex(version);
            return Status::Malformed;
        }
        Value v;
        if (!decode_into(r, v, 0)) {
            err = r.err().empty() ? "Payload decode failed" : r.err();
            return Status::Malformed;
        }
        m.has_body = true;
        m.body = std::move(v);
    }
    consumed = total;
    out = std::move(m);
    return Status::Ok;
}

// ----------------------------------------------------------------- 调试 ------

std::string describe(const Value& v, std::size_t budget)
{
    switch (v.type) {
    case Type::Null:
        return "null";
    case Type::Bool:
        return v.boolean ? "true" : "false";
    case Type::Int64:
        return i64_to_string(v.int64);
    case Type::UInt64:
    case Type::Date:
        return u64_to_string(v.uint64);
    case Type::Double: {
        char buf[40];
        std::snprintf(buf, sizeof(buf), "%g", v.real);
        return buf;
    }
    case Type::String: {
        constexpr std::size_t kTruncate = 120;
        if (v.string.size() <= kTruncate) {
            return '"' + v.string + '"';
        }
        return '"' + v.string.substr(0, kTruncate) + "...\"(" + u64_to_string(v.string.size()) + ")";
    }
    case Type::Data:
        return "<" + u64_to_string(v.data.size()) + " bytes>";
    case Type::FileTransfer:
        return "<file " + u64_to_string(v.file_size) + " bytes, id=" + u64_to_string(v.transfer_id) + (v.data.empty() ? "" : ", received ")
               + ">";
    case Type::Uuid: {
        static constexpr char kHex[] = "0123456789abcdef";
        std::string s;
        for (std::size_t i = 0; i < v.data.size(); ++i) {
            if (i == 4 || i == 6 || i == 8 || i == 10) {
                s += '-';
            }
            s += kHex[v.data[i] >> 4];
            s += kHex[v.data[i] & 0xF];
        }
        return s;
    }
    case Type::Array: {
        std::string s = "[";
        for (std::size_t i = 0; i < v.array.size(); ++i) {
            if (i) {
                s += ", ";
            }
            s += describe(v.array[i], budget);
            if (s.size() > budget) {
                s += ", ...";
                break;
            }
        }
        return s + ']';
    }
    case Type::Dict: {
        std::string s = "{";
        for (std::size_t i = 0; i < v.dict.size(); ++i) {
            if (i) {
                s += ", ";
            }
            s += v.dict[i].key;
            s += ": ";
            s += describe(v.dict[i].value, budget);
            if (s.size() > budget) {
                s += ", ...";
                break;
            }
        }
        return s + '}';
    }
    }
    return "?";
}

} // namespace maa::ios::xpc
