#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace maa::ios::xpc
{

/// XPC 对象与 RemoteXPC 附件共用的输入上限，属于当前应用的资源策略。
/// 已验证的截图可内联多 MiB 数据；超过此限制的载荷需另行评估，不能据此
/// 判定为协议本身不合法。
inline constexpr std::size_t kMaxBuffer = 32u << 20;

/// Apple XPC 的二进制对象图，RemoteXPC 的载荷格式。
///
/// 当前适配支持下面列出的对象类型，以及 RemoteXPC 的消息信封。
/// FileTransfer 只在对象图中编码传输号和长度，文件字节由通道层另行接收。
///
/// 线上格式（全小端）：每个对象 = u32 类型标记 + 该类型的载荷。所有对象的总
/// 长度都是 4 的倍数，字符串与数据后面补零到 4 字节边界，字典的键是「裸的」
/// NUL 结尾字符串同样补到 4 字节边界（注意它和字符串对象不同，**没有**长度前缀）。
enum class Type : uint32_t
{
    Null = 0x00001000,
    Bool = 0x00002000,
    Int64 = 0x00003000,
    UInt64 = 0x00004000,
    Double = 0x00005000,
    Date = 0x00007000,
    Data = 0x00008000,
    String = 0x00009000,
    Uuid = 0x0000A000,
    Array = 0x0000E000,
    Dict = 0x0000F000,
    /// 文件附件以 FileTransfer 声明长度，实际字节在另一条 HTTP/2 流上传输。
    /// 截图也可能直接返回内联 Data，不能仅凭用途判断载荷类型。
    FileTransfer = 0x0001A000,
};

struct Value;

using Array = std::vector<Value>;

/// 字典条目先声明、在 Value 之后定义，以支持两者相互包含。
/// std::vector 允许此处使用尚未完整定义的 Entry 元素类型。
struct Entry;
using Dict = std::vector<Entry>;

struct Value
{
    Type type = Type::Null;

    // type 决定当前值使用哪个字段；其余字段保持默认值。
    bool boolean = false;
    int64_t int64 = 0;
    uint64_t uint64 = 0; ///< UInt64 与 Date 共用；Date 是自 epoch 起的纳秒。
    double real = 0;
    std::string string;
    std::vector<uint8_t> data; ///< Data；Uuid 时长度为 16。
    Array array;
    Dict dict;

    /// 仅用于 FileTransfer：file_size 是元数据字典中 "s" 声明的字节数，
    /// transfer_id 是传输号。实际文件字节存入 data，由通道层接收后填充。
    /// 编解码 FileTransfer 只处理元数据，不读取或写入 data。
    uint64_t file_size = 0;
    uint64_t transfer_id = 0;

    [[nodiscard]] bool is_dict() const { return type == Type::Dict; }

    [[nodiscard]] bool is_array() const { return type == Type::Array; }

    [[nodiscard]] bool is_string() const { return type == Type::String; }

    /// 找不到键返回 nullptr；重复键时取第一个，可用于判断键是否存在。
    [[nodiscard]] const Value* find(std::string_view key) const;
    /// 找不到键返回共享的 Null 值引用，便于读取可选字段并使用默认值。
    /// 需要区分缺失键和显式 Null 值时使用 find。
    [[nodiscard]] const Value& at(std::string_view key) const;
    [[nodiscard]] std::string as_string_or(std::string_view fallback = { }) const;
    [[nodiscard]] int64_t as_int_or(int64_t fallback = 0) const;
    [[nodiscard]] bool as_bool_or(bool fallback = false) const;
};

/// 条目保持插入顺序，编码时沿用该顺序，便于字节级往返比较。
struct Entry
{
    std::string key;
    Value value;
};

// ------------------------------------------------------------- builders ------
Value make_null();
Value make_bool(bool v);
Value make_int64(int64_t v);
Value make_uint64(uint64_t v);
Value make_double(double v);
Value make_date(uint64_t ns_since_epoch);
Value make_string(std::string v);
Value make_data(std::vector<uint8_t> v);
/// 构造 Uuid 值；编码要求 data 恰好为 16 字节，否则编码失败。
Value make_uuid(std::span<const uint8_t> v);
Value make_file_transfer(uint64_t size, uint64_t transfer_id = 0);
Value make_array();
Value make_dict();

void array_push(Value& arr, Value item);
/// 键存在时原地替换第一个条目，否则追加；后续同名条目保持不变。
void dict_set(Value& dict, std::string key, Value item);

// ------------------------------------------------------------ 对象编解码 ------
/// 把对象及其类型标记序列化。递归深度超过 64、Uuid 长度不符或类型不支持时返回空。
[[nodiscard]] std::vector<uint8_t> encode(const Value& v);

/// 从缓冲区开头解出一个对象，不要求对象用尽整个缓冲区。
/// 输入超过 kMaxBuffer、递归深度超过 64 或解析失败时返回 nullopt 并给出原因。
[[nodiscard]] std::optional<Value> decode(std::span<const uint8_t> buf, std::string& err);

// ---------------------------------------------------------- 消息（wrapper） ---
inline constexpr uint32_t kWrapperMagic = 0x29B00B92;
inline constexpr uint32_t kPayloadMagic = 0x42133742;
inline constexpr uint32_t kProtocolVersion = 5;

/// 当前 RemoteXPC 适配使用的信封标志，可以组合设置。
inline constexpr uint32_t kFlagAlwaysSet = 0x00000001;
inline constexpr uint32_t kFlagPing = 0x00000002;
inline constexpr uint32_t kFlagDataPresent = 0x00000100;
/// 当前主通道握手的终止消息使用此标志，组合值为 0x0201（含 ALWAYS_SET）。
/// 该标志与回信标志 kFlagIsReply 独立。
inline constexpr uint32_t kFlagTermChannel = 0x00000200;
inline constexpr uint32_t kFlagWantingReply = 0x00010000;
inline constexpr uint32_t kFlagIsReply = 0x00020000;
inline constexpr uint32_t kFlagFileTxRequest = 0x00100000;
inline constexpr uint32_t kFlagFileTxResponse = 0x00200000;
inline constexpr uint32_t kFlagInitHandshake = 0x00400000;

/// 一条 RemoteXPC 消息。信封布局（全小端）：
///
/// ```text
///  0  u32  wrapper magic 0x29B00B92
///  4  u32  flags
///  8  u64  载荷长度 L        ← 不含 24 字节信封，整条消息 = 24 + L
/// 16  u64  message_id
/// 24       载荷：u32 magic 0x42133742 + u32 版本 5 + 一个 XPC 对象
/// ```
///
/// message_id 属于信封，不计入载荷长度。载荷非空时包含 magic、版本和一个 XPC 对象。
struct Message
{
    uint32_t flags = kFlagAlwaysSet;
    uint64_t message_id = 0;
    bool has_body = false;
    Value body;
};

/// body 为 nullptr 时编码为长度 0、无载荷的消息，用于心跳和握手终止。
[[nodiscard]] std::vector<uint8_t> encode_message(uint32_t flags, uint64_t message_id, const Value* body);

enum class Status
{
    Ok,
    NeedMore,
    Malformed
};

/// 从缓冲区开头解出一条消息。Ok 时 consumed 是本条消息的字节数，其余状态为 0。
/// 一个 DATA 帧可能包含多条消息，因此 consumed 可以小于 buf.size()。
/// NeedMore 表示需要继续接收；Malformed 表示长度超限、格式或版本不受支持等解析错误。
[[nodiscard]] Status decode_message(std::span<const uint8_t> buf, Message& out, std::size_t& consumed, std::string& err);

// ----------------------------------------------------------------- 调试 ------
/// 单行可读表示，用于诊断输出。字符串最多显示前 120 字节。
/// budget 分别应用于每个容器；追加条目后超过预算时以 `, ...` 结束，
/// 因此不是输出长度的严格上限。传 SIZE_MAX 可取消容器预算，字符串仍按上述规则截断。
[[nodiscard]] std::string describe(const Value& v, std::size_t budget = 400);

} // namespace maa::ios::xpc
