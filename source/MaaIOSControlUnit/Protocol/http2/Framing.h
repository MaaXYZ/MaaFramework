#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <vector>

/// RemoteXPC 使用的 HTTP/2 帧子集，不实现 HPACK。
///
/// 客户端建立流时发送空 HEADERS + END_HEADERS，路由由流号决定：
/// 1 为主通道、3 为回信通道，XPC 消息放在 DATA 帧。当前适配不消费入站 header block。
///
/// nghttp2 使用 MIT 许可，已完成适配验证。控制流可用，偶数文件流与其客户端
/// 会话模型不兼容，因此生产保留该帧层。验证范围见 docs/NGHTTP2_COMPATIBILITY.md。
namespace maa::ios::http2
{

inline constexpr std::size_t kFrameHeaderSize = 9;
/// 本实现接收单帧最多 4 MiB。协议长度字段为 24 位，可表示到 16 MiB - 1；
/// 本地上限用于限制外部输入的缓冲分配。
inline constexpr std::size_t kMaxFrameSize = 1u << 22;
/// 协议长度字段和 SETTINGS_MAX_FRAME_SIZE 的上限，与本地接收上限分别维护。
inline constexpr uint32_t kProtocolMaxFrameSize = (1u << 24) - 1;
inline constexpr int64_t kMaxWindowSize = 0x7FFFFFFF;
/// 双方未通过 SETTINGS 协商前的默认值（RFC 7540 §6.5.2 / §6.9.2）。
inline constexpr uint32_t kDefaultInitialWindowSize = 65535;
inline constexpr std::size_t kDefaultMaxFrameSize = 16384;

/// 客户端前置签名：连接上最先写的 24 字节，不是帧。
constexpr const char kClientPreface[] = "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n";
inline constexpr std::size_t kClientPrefaceSize = 24;

enum Type : uint8_t
{
    kData = 0x0,
    kHeaders = 0x1,
    kPriority = 0x2,
    kRstStream = 0x3,
    kSettings = 0x4,
    kPushPromise = 0x5,
    kPing = 0x6,
    kGoAway = 0x7,
    kWindowUpdate = 0x8,
    kContinuation = 0x9,
};

/// 标志位使用 kFlag 前缀，与帧类型名称区分。ACK 和 END_STREAM 的值均为 0x1，
/// 含义由帧类型决定：ACK 用于 SETTINGS / PING，END_STREAM 用于 DATA / HEADERS。
enum Flag : uint8_t
{
    kFlagEndStream = 0x1,
    kFlagAck = 0x1,
    kFlagEndHeaders = 0x4,
    kFlagPadded = 0x8,
    kFlagPriority = 0x20,
};

enum Setting : uint16_t
{
    kSettingHeaderTableSize = 0x1,
    kSettingEnablePush = 0x2,
    kSettingMaxConcurrentStreams = 0x3,
    kSettingInitialWindowSize = 0x4,
    kSettingMaxFrameSize = 0x5,
};

/// RFC 7540 §7 的错误码，出现在 GOAWAY / RST_STREAM 里。
enum ErrorCode : uint32_t
{
    kNoError = 0x0,
    kProtocolError = 0x1,
    kInternalError = 0x2,
    kFlowControlError = 0x3,
    kFrameSizeError = 0x5,
    kCompressionError = 0x9,
};

const char* frame_type_name(uint8_t type);
const char* error_code_name(uint32_t code);

/// 一个已经收全的帧。载荷按原样保留，由调用方按 type 再解。
struct Frame
{
    uint8_t type = kData;
    uint8_t flags = 0;
    uint32_t stream_id = 0;
    std::vector<uint8_t> payload;
};

enum class Status
{
    Ok,
    NeedMore,
    Malformed
};

/// 序列化一个帧头 + 载荷。
[[nodiscard]] std::vector<uint8_t> serialize(uint8_t type, uint8_t flags, uint32_t stream_id, std::span<const uint8_t> payload = { });

// ------------------------------------------------------------ 便捷构造 ------
[[nodiscard]] std::vector<uint8_t> settings_frame(const std::vector<std::pair<uint16_t, uint32_t>>& settings);
[[nodiscard]] std::vector<uint8_t> settings_ack_frame();
[[nodiscard]] std::vector<uint8_t> window_update_frame(uint32_t stream_id, uint32_t increment);
[[nodiscard]] std::vector<uint8_t> headers_frame(uint32_t stream_id);
[[nodiscard]] std::vector<uint8_t> data_frame(uint32_t stream_id, std::span<const uint8_t> payload, uint8_t extra_flags = 0);
[[nodiscard]] std::vector<uint8_t> ping_frame(uint64_t opaque_data, bool ack);

// 从缓冲开头解析一帧，Ok 时 consumed 为帧长度，缓冲可能还包含后续帧。
// NeedMore 表示应继续接收；Malformed 表示长度等输入不满足本实现约束。
Status parse_frame(std::span<const uint8_t> buf, Frame& out, std::size_t& consumed, std::string& err);

/// 解析 DATA 的 PADDED 字段，移除 Pad Length 字节及尾部填充。
/// DATA 未定义的标志位忽略，不将其他帧的可选字段当作 DATA 前缀。
bool data_payload(const Frame& f, std::span<const uint8_t>& out, std::string& err);

/// SETTINGS 载荷 -> (标识符, 值) 列表；`ack` 回带 ACK 标志。
bool parse_settings(const Frame& f, std::vector<std::pair<uint16_t, uint32_t>>& out, bool& ack, std::string& err);

/// WINDOW_UPDATE 的增量。stream 0 是全连接，其余是单流。
bool parse_window_update(const Frame& f, uint32_t& increment, std::string& err);

struct GoAway
{
    uint32_t last_stream_id = 0;
    uint32_t error_code = 0;
    std::string debug_data; // 对端提供的调试文本（解析时过滤控制字符）。
};

bool parse_goaway(const Frame& f, GoAway& out, std::string& err);

bool parse_rst_stream(const Frame& f, uint32_t& error_code, std::string& err);

[[nodiscard]] std::string describe(const Frame& f);

} // namespace maa::ios::http2
