#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <istream>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "http2/Framing.h"
#include "net/ByteStream.h"
#include "xpc/XpcValue.h"

namespace maa::ios::remote
{

/// RSD 使用的客户端身份。
///
/// 已有设备测试中，更换 UUID 可能触发 peer 重新 attach，使已公布的服务端口失效。
/// 调用方应复用配对记录中的稳定身份；不要为每个服务连接临时生成 UUID。
/// 这是当前适配的设备行为，不能据此推定所有 iOS 版本的连接策略。
struct PeerIdentity
{
    std::array<uint8_t, 16> uuid { };
    /// 当前 RemoteXPC 握手使用的版本标志。缺失或不兼容时，设备可返回
    /// "Invalid or missing remote device connection version flags"。
    uint64_t version_flags = 0x0100000000000006ULL;
    uint64_t messaging_protocol_version = 7;
};

/// 解析 UUID 的 32 个十六进制数字，按文本顺序每两个数字组成一个字节。
/// 允许连字符；不按平台 UUID 结构中的字段端序重排。
[[nodiscard]] std::optional<std::array<uint8_t, 16>> parse_uuid_text(std::string_view text);

/// USB 配对的 HostID 不一定是 UUID。已有 UUID 保持原值，其他非空身份按固定
/// 命名空间生成 UUIDv5，保证同一配对记录在不同进程和平台上使用相同的 RSD 身份。
/// 不修改传给 lockdown 的原始 HostID；空值或摘要计算失败时返回 nullopt。
[[nodiscard]] std::optional<std::array<uint8_t, 16>> peer_uuid_from_host_id(std::string_view host_id);

/// 一条 TCP 连接上的 RemoteXPC 通道，负责 HTTP/2 帧和 XPC 消息。
/// 每个 DDI 服务在 RSD 中有独立端口，需要建立自己的连接并完成握手。
/// ByteStream 由调用方持有，必须比 Channel 活得更久。
class Channel
{
public:
    /// 完成 HTTP/2 握手，建立流并等待对端 SETTINGS。
    ///
    /// 当前设备要求 stream 1 的 HEADERS 先于 TermChannel，stream 3 的 HEADERS
    /// 先于 InitHandshake。顺序依据已验证的 RemoteXPC 握手，不能按普通 HTTP 请求重排。
    static std::optional<Channel> open(net::ByteStream& socket, std::string& err, bool verbose = false);

    /// 在 RSD 通道申报身份并读取 peer_info。服务连接不执行身份申报，
    /// 其首条 XPC 消息应是服务自身的请求。
    bool announce_device(const PeerIdentity& identity, std::string& err);

    /// 发送请求；want_reply 决定是否携带 WANTING_REPLY 标志。
    bool send_request(const xpc::Value& body, bool want_reply, std::string& err);

    /// Push an announced attachment on a new client-initiated odd stream. The
    /// FileTxRequest preamble carries transfer_id; bytes follow without waiting
    /// for an acknowledgement. Failure makes the channel unusable; never retry.
    bool send_file_transfer(
        uint64_t transfer_id,
        uint64_t size,
        std::istream& input,
        int timeout_ms,
        const std::function<bool()>& cancelled,
        const std::function<void(uint64_t, uint64_t)>& progress,
        std::string& err);

    /// 区分已收到消息、等待超时和连接或协议错误。
    enum class Wait
    {
        Message,
        Timeout,
        Broken
    };

    /// 取回下一条带字典载荷的消息（心跳、空载荷帧、控制帧会被跳过）。
    bool receive(xpc::Value& out, int timeout_ms, std::string& err) { return wait(out, timeout_ms, err) == Wait::Message; }

    /// 等待非空字典消息。Timeout 表示本轮未收到消息；Broken 表示连接或协议失败。
    /// 常驻订阅可能长期没有更新，调用方应仅在 Broken 时按恢复策略重连。
    Wait wait(xpc::Value& out, int timeout_ms, std::string& err);

    /// 一次往返。
    bool call(const xpc::Value& request, xpc::Value& reply, int timeout_ms, std::string& err);

    /// 处理连接上的 HTTP/2 控制帧，即使当前没有业务消息也需调用。
    /// 会应答 PING、处理 WINDOW_UPDATE，并缓冲收到的 XPC 数据。
    /// 返回 true 表示未发现连接错误，包括读超时；不保证本轮有数据或进展。
    bool service(int timeout_ms, std::string& err) { return pump(timeout_ms, err); }

    /// 设备在握手时自报的身份：Model / OSVersion / Udid / Properties 等。
    [[nodiscard]] const xpc::Value* peer_info() const { return peer_info_.has_value() ? &*peer_info_ : nullptr; }

    [[nodiscard]] net::ByteStream& socket() { return socket_; }

    /// 主通道 / 回信通道的流号。1 发请求，3 收异步回信（服务连接上设备只用 1）。
    static constexpr uint32_t kRootStream = 1;
    static constexpr uint32_t kReplyStream = 3;

    /// 构造函数公开供 std::optional::emplace 使用。直接构造的对象尚未握手；
    /// 建立可用通道应调用 open()。
    explicit Channel(net::ByteStream& socket)
        : socket_(socket)
    {
    }

private:
    bool start(std::string& err);
    bool send_data(
        uint32_t stream_id,
        std::span<const uint8_t> payload,
        std::string& err,
        std::optional<std::chrono::steady_clock::time_point> deadline = { },
        const std::function<bool()>& cancelled = { });
    /// 处理已缓冲的完整帧，或读取一批字节。连接、解析或控制帧处理失败返回 false；
    /// 读超时返回 true，由调用方使用总 deadline 判断等待是否结束。
    bool pump(int timeout_ms, std::string& err);
    bool handle_frame(const http2::Frame& f, std::string& err);
    /// 接受设备文件流，并等待 size 字节；与业务回复共用同一 deadline。
    bool receive_file(
        uint32_t stream_id,
        uint64_t size,
        std::chrono::steady_clock::time_point deadline,
        std::vector<uint8_t>& out,
        std::string& err);
    /// 递归收集字典/数组里所有 FileTransfer 占位，顺序即流号顺序。
    static bool collect_files(xpc::Value& v, std::vector<xpc::Value*>& out);
    std::size_t buffered_bytes() const;
    /// 接收 FileTransfer 引用的文件，填入其 data 字段。
    bool materialize_files(xpc::Value& reply, std::chrono::steady_clock::time_point deadline, std::string& err);
    /// 从各流缓冲中读取一条回复，并接收其引用的文件。
    /// false 且 err 为空表示消息尚未收全；err 非空表示解析或文件接收失败。
    bool take_message(xpc::Value& out, std::chrono::steady_clock::time_point deadline, std::string& err);
    bool replenish_inbound_window(uint32_t stream_id, std::string& err);
    /// 发送完整帧；verbose 模式记录原始字节，供协议排查。
    bool send_bytes(std::span<const uint8_t> data, std::string& err);

    net::ByteStream& socket_;
    std::vector<uint8_t> rx_; ///< 还没凑成一帧的原始字节
    /// 累计接收字节数。减去 rx_.size() 得到当前缓冲开头在原始入流中的偏移，
    /// 用于对照 MAA_IOS_H2_DUMP 文件定位解析错误。
    uint64_t rx_total_ = 0;

    /// MAA_IOS_H2_DUMP 指定文件前缀；每条连接将原始入流保存为 <prefix>.<n>.bin。
    /// 可配合解析错误中的流内偏移离线重放。文件内容可能包含设备及业务数据。
    /// Channel 可移动，使用 unique_ptr 保证文件句柄只由一个对象关闭。
    struct FileCloser
    {
        int operator()(FILE* f) const { return std::fclose(f); }
    };

    std::unique_ptr<FILE, FileCloser> dump_ { nullptr };
    /// 每条连接仅在第一次 pump 时读取 dump 配置。
    bool opened_dump_ = false;
    /// 开 dump 文件（见 `dump_`）。
    void maybe_open_dump();
    /// 按流号分别缓冲 XPC 字节；同一消息可能跨多个 DATA 帧。
    std::map<uint32_t, std::vector<uint8_t>> pending_;

    /// 当前文件传输约定在偶数流发送原始文件字节，单独缓冲，不经过 XPC 解码。
    // 这些是当前适配的资源限制，并未实现通用 HTTP/2 流状态机。
    // 每条连接最多保留 100 个文件流记录，包含已消费的空记录。现有回复内
    // 2、4、6 映射不能安全复用流号，保留空记录用于拒绝重复使用。
    // FileTransfer 的真机互操作仍需回归验证。
    struct FileStream
    {
        std::vector<uint8_t> bytes;
        std::optional<uint64_t> expected;
        bool ended = false;
        bool consumed = false;
    };

    std::map<uint32_t, FileStream> raw_;
    std::optional<xpc::Value> peer_info_;
    uint64_t next_message_id_ = 0;
    uint32_t next_outbound_file_stream_ = 5;
    std::set<uint32_t> finished_outbound_files_;
    std::set<uint64_t> outbound_transfer_ids_;

    // 流控：我方发出去的量受对端授权，对端发进来的量由我们授予。
    uint32_t peer_initial_window_ = http2::kDefaultInitialWindowSize;
    int64_t outbound_connection_ = http2::kDefaultInitialWindowSize;
    std::map<uint32_t, int64_t> outbound_streams_;
    uint32_t peer_max_frame_ = static_cast<uint32_t>(http2::kDefaultMaxFrameSize);
    uint64_t consumed_connection_ = 0;
    std::map<uint32_t, uint64_t> consumed_per_stream_;

    /// 保存对端 SETTINGS 内容。settings_received_ 单独记录是否收到非 ACK 帧，
    /// 因为空 SETTINGS 也合法，不能使用条目数量判断握手完成。
    std::vector<std::pair<uint16_t, uint32_t>> peer_settings_;
    bool settings_received_ = false;
    bool terminated_ = false;
    bool verbose_ = false;
};

} // namespace maa::ios::remote
