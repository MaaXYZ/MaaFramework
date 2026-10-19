#include "remote/RemoteXpc.h"

#include "MaaUtils/Logger.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <openssl/evp.h>
#include <string_view>

namespace maa::ios::remote
{
namespace
{

/// 每条流的初始接收窗口为 16 MiB。服务目录可能超过默认的 65535 字节，
/// 较大的窗口减少接收大回复时的流控等待。
constexpr uint32_t kGrantWindow = 16u << 20;
constexpr uint32_t kWindowIncr = kGrantWindow - http2::kDefaultInitialWindowSize;

/// 累计消费 1 MiB 后补充窗口，减少 WINDOW_UPDATE 帧数量。
constexpr uint64_t kReplenishThreshold = 1u << 20;
constexpr std::size_t kMaxFileStreams = 100;
constexpr std::size_t kMaxXpcStream = xpc::kMaxBuffer + 24;
constexpr std::size_t kMaxBuffered = 2 * xpc::kMaxBuffer;

bool write_all(net::ByteStream& sock, std::span<const uint8_t> data, std::string& err)
{
    return sock.send(std::string_view(reinterpret_cast<const char*>(data.data()), data.size()), err);
}

bool write_all(net::ByteStream& sock, std::string_view data, std::string& err)
{
    return sock.send(data, err);
}

/// 非空载荷设置 DATA_PRESENT。当前 RemoteXPC 约定中，空字典仅设置
/// ALWAYS_SET；WANTING_REPLY 独立由调用方指定。
uint32_t wrapper_flags(const xpc::Value* body, bool want_reply)
{
    uint32_t flags = xpc::kFlagAlwaysSet;
    if (body != nullptr && (body->type != xpc::Type::Dict || !body->dict.empty())) {
        flags |= xpc::kFlagDataPresent;
    }
    if (want_reply) {
        flags |= xpc::kFlagWantingReply;
    }
    return flags;
}

xpc::Value build_handshake(const PeerIdentity& identity)
{
    auto d = xpc::make_dict();
    xpc::dict_set(d, "MessageType", xpc::make_string("Handshake"));
    xpc::dict_set(d, "MessagingProtocolVersion", xpc::make_uint64(identity.messaging_protocol_version));
    xpc::dict_set(d, "UUID", xpc::make_uuid(std::span<const uint8_t>(identity.uuid.data(), identity.uuid.size())));
    auto props = xpc::make_dict();
    xpc::dict_set(props, "RemoteXPCVersionFlags", xpc::make_uint64(identity.version_flags));
    // 请求展示服务发现所需的敏感属性。
    xpc::dict_set(props, "SensitivePropertiesVisible", xpc::make_bool(true));
    xpc::dict_set(d, "Properties", std::move(props));
    xpc::dict_set(d, "Services", xpc::make_dict());
    return d;
}

int64_t remaining_ms(std::chrono::steady_clock::time_point deadline)
{
    const auto left = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - std::chrono::steady_clock::now()).count();
    return left > 0 ? left : 0;
}

int nibble_value(char c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    return -1;
}

} // namespace

std::optional<std::array<uint8_t, 16>> parse_uuid_text(std::string_view text)
{
    // 32 个十六进制字符，允许中间夹 '-'（标准 8-4-4-4-12 写法）。
    std::array<uint8_t, 16> out { };
    std::size_t digits = 0;
    for (const char c : text) {
        if (c == '-') {
            continue;
        }
        const int v = nibble_value(c);
        if (v < 0) {
            return std::nullopt;
        }
        // 写入前检查数字数量，避免超过 16 字节数组。
        if (digits >= out.size() * 2) {
            return std::nullopt;
        }
        out[digits / 2] = static_cast<uint8_t>(digits % 2 == 0 ? (v << 4) : (out[digits / 2] | v));
        ++digits;
    }
    if (digits != 32) {
        return std::nullopt;
    }
    return out;
}

std::optional<std::array<uint8_t, 16>> peer_uuid_from_host_id(std::string_view host_id)
{
    if (host_id.empty()) {
        return std::nullopt;
    }
    if (auto uuid = parse_uuid_text(host_id)) {
        return uuid;
    }

    // UUIDv5 = SHA-1(namespace || name)，按 RFC 9562 设置版本和 variant。
    // URL 命名空间与用途前缀共同定义这条映射，后续不能随意变更，否则会改变
    // 已配对设备的 RSD 身份。SHA-1 仅用于名称映射，不用于认证或密钥派生。
    constexpr std::array<uint8_t, 16> namespace_url { 0x6b, 0xa7, 0xb8, 0x11, 0x9d, 0xad, 0x11, 0xd1,
                                                      0x80, 0xb4, 0x00, 0xc0, 0x4f, 0xd4, 0x30, 0xc8 };
    constexpr std::string_view prefix = "MaaFramework:ios:usbmux:HostID:";
    std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> ctx(EVP_MD_CTX_new(), EVP_MD_CTX_free);
    std::array<uint8_t, EVP_MAX_MD_SIZE> digest { };
    unsigned int length = 0;
    if (!ctx || EVP_DigestInit_ex(ctx.get(), EVP_sha1(), nullptr) != 1
        || EVP_DigestUpdate(ctx.get(), namespace_url.data(), namespace_url.size()) != 1
        || EVP_DigestUpdate(ctx.get(), prefix.data(), prefix.size()) != 1
        || EVP_DigestUpdate(ctx.get(), host_id.data(), host_id.size()) != 1 || EVP_DigestFinal_ex(ctx.get(), digest.data(), &length) != 1
        || length != 20) {
        return std::nullopt;
    }
    std::array<uint8_t, 16> uuid { };
    std::copy_n(digest.begin(), uuid.size(), uuid.begin());
    uuid[6] = static_cast<uint8_t>((uuid[6] & 0x0f) | 0x50);
    uuid[8] = static_cast<uint8_t>((uuid[8] & 0x3f) | 0x80);
    return uuid;
}

bool Channel::send_bytes(std::span<const uint8_t> data, std::string& err)
{
    if (verbose_) {
        std::string hex;
        hex.reserve(data.size() * 2);
        for (const uint8_t b : data) {
            hex += std::format("{:02x}", b);
        }
        LogTrace << "HTTP/2 send" << VAR(data.size()) << hex;
    }
    return write_all(socket_, data, err);
}

std::size_t Channel::buffered_bytes() const
{
    std::size_t total = rx_.size();
    for (const auto& [id, bytes] : pending_) {
        total += bytes.size();
    }
    for (const auto& [id, file] : raw_) {
        total += file.bytes.size();
    }
    return total;
}

bool Channel::start(std::string& err)
{
    outbound_streams_.emplace(kRootStream, peer_initial_window_);
    outbound_streams_.emplace(kReplyStream, peer_initial_window_);
    // 1. 客户端前置签名（不是帧，24 字节裸串）。
    if (!write_all(socket_, std::string_view(http2::kClientPreface, http2::kClientPrefaceSize), err)) {
        err = "Failed to send HTTP/2 preface: " + err;
        return false;
    }
    // 2. 声明流级接收窗口和并发流数量。
    if (!send_bytes(
            http2::settings_frame({ { http2::kSettingMaxConcurrentStreams, 100 }, { http2::kSettingInitialWindowSize, kGrantWindow } }),
            err)) {
        err = "Failed to send SETTINGS: " + err;
        return false;
    }
    // 3. 将连接级接收窗口增加到 16 MiB。
    //    发送窗口仍使用默认值，直到对端 SETTINGS / WINDOW_UPDATE 更新。
    if (!send_bytes(http2::window_update_frame(0, kWindowIncr), err)) {
        err = "Failed to send WINDOW_UPDATE: " + err;
        return false;
    }
    // 4–8. 按设备要求建立主通道和回信通道。
    //
    // XPC wrapper 必须放入 DATA 帧。直接发送 wrapper 会使其 magic 被当作
    // HTTP/2 帧头，可能表现为帧长度错误。
    if (!send_bytes(http2::headers_frame(kRootStream), err)) {
        err = "Failed to send primary channel HEADERS: " + err;
        return false;
    }
    auto empty_dict = xpc::make_dict();
    auto opener = xpc::encode_message(wrapper_flags(&empty_dict, false), 0, &empty_dict);
    if (!send_data(kRootStream, opener, err)) {
        err = "Failed to send first frame: " + err;
        return false;
    }
    next_message_id_ = 1;
    if (!send_bytes(http2::headers_frame(kReplyStream), err)) {
        err = "Failed to send reply channel HEADERS: " + err;
        return false;
    }
    // 主通道的「终止帧」：空载荷，flags = ALWAYS_SET | 0x200（见常量注释）。
    auto term = xpc::encode_message(xpc::kFlagAlwaysSet | xpc::kFlagTermChannel, 0, nullptr);
    if (!send_data(kRootStream, term, err)) {
        err = "Failed to send terminating frame: " + err;
        return false;
    }
    auto init = xpc::encode_message(xpc::kFlagAlwaysSet | xpc::kFlagInitHandshake, 0, nullptr);
    if (!send_data(kReplyStream, init, err)) {
        err = "Failed to send INIT_HANDSHAKE frame: " + err;
        return false;
    }

    // 9. 等对端 SETTINGS 并 ACK。它可能夹在 WINDOW_UPDATE 后面来，所以按类型筛。
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (!settings_received_) {
        const auto left = remaining_ms(deadline);
        if (left == 0) {
            err = "Timed out waiting for device SETTINGS";
            return false;
        }
        if (!pump(static_cast<int>(left), err)) {
            err = "Disconnected while waiting for device SETTINGS: " + err;
            return false;
        }
    }

    return true;
}

bool Channel::announce_device(const PeerIdentity& identity, std::string& err)
{
    auto hs = build_handshake(identity);
    if (!send_request(hs, false, err)) {
        err = "Failed to send RemoteXPC identity: " + err;
        return false;
    }
    xpc::Value info;
    if (!receive(info, 5000, err)) {
        err = "Failed to read peer_info: " + err;
        return false;
    }
    peer_info_ = std::move(info);
    return true;
}

std::optional<Channel> Channel::open(net::ByteStream& socket, std::string& err, bool verbose)
{
    std::optional<Channel> ch;
    ch.emplace(socket);
    ch->verbose_ = verbose;
    if (!ch->start(err)) {
        return std::nullopt;
    }
    return ch;
}

bool Channel::pump(int timeout_ms, std::string& err)
{
    err.clear();
    if (terminated_) {
        err = "Connection terminated";
        return false;
    }
    maybe_open_dump();
    bool processed = false;
    for (;;) {
        http2::Frame f;
        std::size_t used = 0;
        std::string perr;
        const auto st = http2::parse_frame(rx_, f, used, perr);
        if (st == http2::Status::Ok) {
            rx_.erase(rx_.begin(), rx_.begin() + static_cast<std::ptrdiff_t>(used));
            if (verbose_) {
                LogDebug << std::format("<- {}", http2::describe(f));
            }
            if (!handle_frame(f, err)) {
                // 已消耗导致失败的帧，不能把下一次调用当成同一条健康连接继续。
                terminated_ = true;
                return false;
            }
            processed = true;
            continue;
        }
        if (st == http2::Status::Malformed) {
            terminated_ = true;
            err = perr;
            // 记录缓冲前缀及流内偏移，便于与原始 dump 对照。
            // 这些信息用于定位问题，不足以单独判断是设备、传输还是本地解析错误。
            std::string hex;
            for (std::size_t i = 0; i < 64 && i < rx_.size(); ++i) {
                hex += std::format("{:02x}", rx_[i]);
            }
            LogError << "HTTP/2 parse failed:" << perr << VAR(rx_.size()) << VAR(rx_total_ - rx_.size()) << VAR(rx_total_)
                     << "first 64 bytes:" << hex;
            return false;
        }
        break; // 缓冲里只剩半帧，去收字节
    }
    // 已处理完整帧时先交回控制权，让调用方读取可能已经收全的回复，
    // 避免继续阻塞等待下一批数据。
    if (processed) {
        return true;
    }

    std::vector<uint8_t> got;
    bool timed_out = false;
    if (!socket_.recv(got, timeout_ms, err, &timed_out)) {
        if (timed_out) {
            // 单次读取超时不判定为断连。调用方按自己的总 deadline 决定是否继续等待。
            return true;
        }
        terminated_ = true;
        return false;
    }
    if (got.size() > kMaxBuffered - buffered_bytes()) {
        terminated_ = true;
        err = "RemoteXPC connection input exceeds 64 MiB";
        return false;
    }
    rx_.insert(rx_.end(), got.begin(), got.end());
    rx_total_ += got.size();
    if (dump_ != nullptr) {
        std::fwrite(got.data(), 1, got.size(), dump_.get());
        std::fflush(dump_.get());
    }
    return true;
}

/// 每条连接使用独立 dump 序号，避免不同服务的字节混合。
void Channel::maybe_open_dump()
{
    if (dump_ != nullptr || opened_dump_) {
        return;
    }
    opened_dump_ = true;
    const char* prefix = std::getenv("MAA_IOS_H2_DUMP");
    if (prefix == nullptr || prefix[0] == '\0') {
        return;
    }
    static std::atomic<int> seq { 0 };
    const std::string path = std::string(prefix) + "." + std::to_string(seq.fetch_add(1)) + ".bin";
    dump_.reset(std::fopen(path.c_str(), "wb"));
    if (dump_ != nullptr) {
        LogDebug << std::format("HTTP/2 input dump -> {}", path);
    }
}

bool Channel::handle_frame(const http2::Frame& f, std::string& err)
{
    switch (f.type) {
    case http2::kSettings: {
        bool ack = false;
        if (!http2::parse_settings(f, peer_settings_, ack, err)) {
            return false;
        }
        if (ack) {
            return true;
        }
        for (const auto& [id, value] : peer_settings_) {
            switch (id) {
            case http2::kSettingInitialWindowSize: {
                // RFC 7540 §6.9.2：INITIAL_WINDOW_SIZE 的变化作用于各流的发送窗口，
                // 不修改连接级窗口；后者仅由 WINDOW_UPDATE 更新。
                const int64_t delta = static_cast<int64_t>(value) - static_cast<int64_t>(peer_initial_window_);
                for (const auto& [stream, window] : outbound_streams_) {
                    if (window + delta > http2::kMaxWindowSize) {
                        err = "Flow-control window exceeds 2^31 - 1";
                        return false;
                    }
                }
                peer_initial_window_ = value;
                for (auto& [stream, window] : outbound_streams_) {
                    window += delta;
                }
                break;
            }
            case http2::kSettingMaxFrameSize:
                // 对端可以接受更大的帧，我方仍可选择较小分片。
                peer_max_frame_ = std::min(value, static_cast<uint32_t>(http2::kMaxFrameSize));
                break;
            default:
                break;
            }
        }
        settings_received_ = true;
        return send_bytes(http2::settings_ack_frame(), err);
    }
    case http2::kWindowUpdate: {
        uint32_t inc = 0;
        if (!http2::parse_window_update(f, inc, err)) {
            return false;
        }
        if (f.stream_id == 0) {
            if (outbound_connection_ > http2::kMaxWindowSize - inc) {
                err = "Flow-control window exceeds 2^31 - 1";
                return false;
            }
            outbound_connection_ += inc;
        }
        else {
            auto it = outbound_streams_.find(f.stream_id);
            // 尚未建立或已消费的流不再创建发送窗口状态。
            if (it == outbound_streams_.end()) {
                return true;
            }
            const int64_t base = it->second;
            if (base > http2::kMaxWindowSize - inc) {
                err = "Flow-control window exceeds 2^31 - 1";
                return false;
            }
            it->second = base + inc;
        }
        return true;
    }
    case http2::kPing:
        if (f.stream_id != 0 || f.payload.size() != 8) {
            err = "PING must use stream 0 and contain exactly 8 bytes";
            return false;
        }
        // 非 ACK 的 PING 原样应答，维持设备的连接活性检查。
        if ((f.flags & http2::kFlagAck) != 0) {
            return true;
        }
        {
            uint64_t opaque = 0;
            for (int i = 0; i < 8; ++i) {
                opaque = opaque << 8 | f.payload[static_cast<std::size_t>(i)];
            }
            return send_bytes(http2::ping_frame(opaque, true), err);
        }
    case http2::kGoAway: {
        if (f.stream_id != 0) {
            err = "GOAWAY must use stream 0";
            return false;
        }
        http2::GoAway g;
        if (!http2::parse_goaway(f, g, err)) {
            return false;
        }
        terminated_ = true;
        err = std::string("Device sent GOAWAY ") + http2::error_code_name(g.error_code);
        if (!g.debug_data.empty()) {
            err += "：\"" + g.debug_data + "\"";
        }
        return false;
    }
    case http2::kRstStream: {
        if (f.stream_id == 0) {
            err = "RST_STREAM must use a nonzero stream";
            return false;
        }
        uint32_t code = 0;
        if (!http2::parse_rst_stream(f, code, err)) {
            return false;
        }
        if (finished_outbound_files_.contains(f.stream_id)) {
            return true;
        }
        // 重置也使该流的缓冲数据失效，不能在同批帧报告重置后仍返回回复或文件。
        err = std::string("RemoteXPC stream reset: ") + std::to_string(f.stream_id) + ": " + http2::error_code_name(code);
        return false;
    }
    case http2::kData: {
        if (f.stream_id == 0) {
            err = "DATA must use a nonzero stream";
            return false;
        }
        std::span<const uint8_t> body;
        if (!http2::data_payload(f, body, err)) {
            return false;
        }
        std::vector<uint8_t>* buf = nullptr;
        FileStream* file = nullptr;
        if (f.stream_id % 2 == 0) {
            auto it = raw_.find(f.stream_id);
            if (it == raw_.end()) {
                if (raw_.size() >= kMaxFileStreams) {
                    err = "RemoteXPC file stream limit exceeded (100)";
                    return false;
                }
                it = raw_.try_emplace(f.stream_id).first;
                outbound_streams_.emplace(f.stream_id, peer_initial_window_);
            }
            file = &it->second;
            // 文件按声明长度交付后，允许对端再发一次独立的空 END_STREAM。
            // 这只补齐终结标记；新字节、重复终结或新元数据仍不能复用该流。
            if (file->consumed && !file->ended && f.payload.empty() && (f.flags & http2::kFlagEndStream) != 0) {
                file->ended = true;
                return true;
            }
            if (file->consumed || file->ended) {
                err = "RemoteXPC file stream cannot be reused: " + std::to_string(f.stream_id);
                return false;
            }
            buf = &file->bytes;
            const auto limit = file->expected.value_or(xpc::kMaxBuffer);
            if (body.size() > limit - buf->size()) {
                err = "RemoteXPC file data exceeds declared size or 32 MiB limit";
                return false;
            }
            if ((f.flags & http2::kFlagEndStream) != 0) {
                file->ended = true;
                if (file->expected && buf->size() + body.size() != *file->expected) {
                    err = "RemoteXPC file ended before its declared size";
                    return false;
                }
            }
        }
        else {
            const bool upload_ack = f.stream_id >= 5 && f.stream_id < next_outbound_file_stream_;
            if (f.stream_id != kRootStream && f.stream_id != kReplyStream && !upload_ack) {
                err = "Unsupported RemoteXPC message stream: " + std::to_string(f.stream_id);
                return false;
            }
            buf = &pending_[f.stream_id];
            const std::size_t limit = upload_ack ? 24 : kMaxXpcStream;
            if (body.size() > limit - buf->size()) {
                err = "RemoteXPC message stream input exceeds 32 MiB plus wrapper";
                return false;
            }
        }
        if (body.size() > kMaxBuffered - buffered_bytes()) {
            err = "RemoteXPC connection input exceeds 64 MiB";
            return false;
        }
        buf->insert(buf->end(), body.begin(), body.end());
        // 流控包括 Pad Length 和填充；XPC / 文件缓冲只保存业务载荷。
        consumed_per_stream_[f.stream_id] += f.payload.size();
        consumed_connection_ += f.payload.size();
        if (!replenish_inbound_window(f.stream_id, err)) {
            terminated_ = true;
            return false;
        }
        if ((f.flags & http2::kFlagEndStream) != 0 && (f.stream_id == kRootStream || f.stream_id == kReplyStream)) {
            terminated_ = true;
            err = "Device set END_STREAM on message channel; connection ended";
            return false;
        }
        return true;
    }
    default:
        // 本适配不消费 HEADERS / CONTINUATION / PRIORITY 的载荷；
        // RemoteXPC 根据流号路由 XPC 消息。未识别帧也按当前策略忽略。
        return true;
    }
}

bool Channel::replenish_inbound_window(uint32_t stream_id, std::string& err)
{
    if (consumed_connection_ >= kReplenishThreshold) {
        const auto n = static_cast<uint32_t>(std::min<uint64_t>(consumed_connection_, 0x7FFFFFFF));
        if (!send_bytes(http2::window_update_frame(0, n), err)) {
            err = "Failed to replenish connection receive window: " + err;
            return false;
        }
        consumed_connection_ -= n;
    }
    auto it = consumed_per_stream_.find(stream_id);
    if (it != consumed_per_stream_.end() && it->second >= kReplenishThreshold) {
        const auto n = static_cast<uint32_t>(std::min<uint64_t>(it->second, 0x7FFFFFFF));
        if (!send_bytes(http2::window_update_frame(stream_id, n), err)) {
            err = "Failed to replenish stream receive window: " + err;
            return false;
        }
        it->second -= n;
    }
    return true;
}

bool Channel::send_data(
    uint32_t stream_id,
    std::span<const uint8_t> payload,
    std::string& err,
    std::optional<std::chrono::steady_clock::time_point> deadline,
    const std::function<bool()>& cancelled)
{
    std::size_t off = 0;
    int idle_rounds = 0;
    while (off < payload.size()) {
        if ((cancelled && cancelled()) || (deadline && remaining_ms(*deadline) == 0)) {
            terminated_ = true;
            err = "RemoteXPC upload cancelled or timed out";
            return false;
        }
        auto& stream_window = outbound_streams_.try_emplace(stream_id, peer_initial_window_).first->second;
        const auto budget = std::min({ outbound_connection_, stream_window, static_cast<int64_t>(peer_max_frame_) });
        if (budget <= 0) {
            // 发送窗口耗尽后最多等待五轮，每轮读超时上限 1 秒。
            // 这是轮数限制，实际总时长仍受底层读写耗时影响。
            if (++idle_rounds > 5 && !deadline) {
                terminated_ = true;
                err = "Peer did not replenish flow-control window; sent " + std::to_string(off) + "/" + std::to_string(payload.size())
                      + " bytes";
                return false;
            }
            if (!pump(deadline ? static_cast<int>(std::min<int64_t>(1000, remaining_ms(*deadline))) : 1000, err)) {
                err = "Disconnected while waiting for flow-control window: " + err;
                return false;
            }
            continue;
        }
        idle_rounds = 0;
        const auto n = std::min<uint64_t>(static_cast<uint64_t>(budget), static_cast<uint64_t>(payload.size() - off));
        auto frame = http2::data_frame(stream_id, payload.subspan(off, n));
        if (!send_bytes(frame, err)) {
            terminated_ = true;
            err = "Failed to send DATA: " + err;
            return false;
        }
        outbound_connection_ -= static_cast<int64_t>(n);
        stream_window -= static_cast<int64_t>(n);
        off += static_cast<std::size_t>(n);
    }
    return true;
}

bool Channel::send_file_transfer(
    uint64_t transfer_id,
    uint64_t size,
    std::istream& input,
    int timeout_ms,
    const std::function<bool()>& cancelled,
    const std::function<void(uint64_t, uint64_t)>& progress,
    std::string& err)
{
    err.clear();
    if (terminated_ || timeout_ms <= 0 || transfer_id == 0 || size > (64u << 20) || outbound_transfer_ids_.size() >= kMaxFileStreams
        || outbound_transfer_ids_.contains(transfer_id)) {
        err = "Invalid or reused RemoteXPC upload attachment";
        return false;
    }
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
    if (cancelled && cancelled()) {
        err = "RemoteXPC upload cancelled or timed out";
        return false;
    }
    const uint32_t stream = next_outbound_file_stream_;
    next_outbound_file_stream_ += 2;
    outbound_transfer_ids_.insert(transfer_id);
    outbound_streams_.emplace(stream, peer_initial_window_);
    if (!send_bytes(http2::headers_frame(stream), err)) {
        terminated_ = true;
        return false;
    }
    const auto preamble = xpc::encode_message(xpc::kFlagAlwaysSet | xpc::kFlagFileTxRequest, transfer_id, nullptr);
    if (!send_data(stream, preamble, err, deadline, cancelled)) {
        return false;
    }
    std::array<uint8_t, 64 * 1024> buffer { };
    uint64_t offset = 0;
    while (offset < size) {
        const auto n = static_cast<std::size_t>(std::min<uint64_t>(buffer.size(), size - offset));
        input.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(n));
        if (input.gcount() != static_cast<std::streamsize>(n)) {
            terminated_ = true;
            err = "RemoteXPC upload file ended before its declared size";
            return false;
        }
        if (!send_data(stream, std::span(buffer.data(), n), err, deadline, cancelled)) {
            return false;
        }
        offset += n;
        if (progress) {
            progress(offset, size);
        }
    }
    if ((cancelled && cancelled()) || remaining_ms(deadline) == 0) {
        terminated_ = true;
        err = "RemoteXPC upload cancelled or timed out";
        return false;
    }
    if (!send_bytes(http2::data_frame(stream, { }, http2::kFlagEndStream), err)) {
        terminated_ = true;
        return false;
    }
    finished_outbound_files_.insert(stream);
    outbound_streams_.erase(stream);
    return true;
}

bool Channel::send_request(const xpc::Value& body, bool want_reply, std::string& err)
{
    err.clear();
    if (terminated_) {
        err = "Connection terminated";
        return false;
    }
    const auto flags = wrapper_flags(&body, want_reply);
    auto wire = xpc::encode_message(flags, next_message_id_, &body);
    if (wire.empty()) {
        err = "XPC encoding failed (depth or length limit exceeded)";
        return false;
    }
    if (verbose_) {
        LogDebug << std::format("-> id={} flags=0x{:x} {}", next_message_id_, flags, xpc::describe(body).substr(0, 200));
    }
    if (!send_data(kRootStream, wire, err)) {
        return false;
    }
    ++next_message_id_;
    return true;
}

bool Channel::collect_files(xpc::Value& v, std::vector<xpc::Value*>& out)
{
    if (v.type == xpc::Type::FileTransfer) {
        if (out.size() == kMaxFileStreams) {
            return false;
        }
        out.push_back(&v);
    }
    for (auto& entry : v.dict) {
        if (!collect_files(entry.value, out)) {
            return false;
        }
    }
    for (auto& item : v.array) {
        if (!collect_files(item, out)) {
            return false;
        }
    }
    return true;
}

bool Channel::receive_file(
    uint32_t stream_id,
    uint64_t size,
    std::chrono::steady_clock::time_point deadline,
    std::vector<uint8_t>& out,
    std::string& err)
{
    auto& file = raw_.at(stream_id);
    if (size != 0) {
        // 保留现有 RemoteXPC 接受顺序与偶数文件流映射。
        std::vector<uint8_t> frames = http2::headers_frame(stream_id);
        auto accept = xpc::encode_message(xpc::kFlagAlwaysSet | xpc::kFlagFileTxResponse, 0, nullptr);
        auto ack_frame = http2::data_frame(stream_id, accept);
        frames.insert(frames.end(), ack_frame.begin(), ack_frame.end());
        if (!send_bytes(frames, err)) {
            err = "Failed to accept file stream: " + err;
            return false;
        }
    }
    while (file.bytes.size() < size) {
        if (file.ended) {
            err = "RemoteXPC file ended before its declared size";
            return false;
        }
        const auto left = remaining_ms(deadline);
        if (left == 0) {
            err = "File receive timed out: stream " + std::to_string(stream_id) + " has received " + std::to_string(file.bytes.size()) + "/"
                  + std::to_string(size) + " bytes";
            return false;
        }
        if (!pump(static_cast<int>(left), err)) {
            err = "Disconnected while receiving file: " + err;
            return false;
        }
    }
    out.clear();
    out.swap(file.bytes);
    file.consumed = true;
    consumed_per_stream_.erase(stream_id);
    outbound_streams_.erase(stream_id);
    return true;
}

bool Channel::materialize_files(xpc::Value& reply, std::chrono::steady_clock::time_point deadline, std::string& err)
{
    std::vector<xpc::Value*> files;
    if (!collect_files(reply, files)) {
        err = "RemoteXPC file stream limit exceeded (100)";
        return false;
    }
    uint64_t total = 0;
    // 先检查整条回复的附件，再发送接受帧或接收文件数据。
    for (std::size_t i = 0; i < files.size(); ++i) {
        const auto size = files[i]->file_size;
        if (size > xpc::kMaxBuffer || size > xpc::kMaxBuffer - total) {
            err = "RemoteXPC reply attachments exceed 32 MiB";
            return false;
        }
        total += size;
        const auto id = static_cast<uint32_t>((i + 1) * 2);
        auto it = raw_.find(id);
        if (it == raw_.end()) {
            if (raw_.size() >= kMaxFileStreams) {
                err = "RemoteXPC file stream limit exceeded (100)";
                return false;
            }
            it = raw_.try_emplace(id).first;
            outbound_streams_.emplace(id, peer_initial_window_);
        }
        auto& file = it->second;
        if (file.consumed || file.expected) {
            err = "RemoteXPC file stream cannot be reused: " + std::to_string(id);
            return false;
        }
        if (file.bytes.size() > size) {
            err = "RemoteXPC file data exceeds declared size or 32 MiB limit";
            return false;
        }
        if (file.ended && file.bytes.size() != size) {
            err = "RemoteXPC file ended before its declared size";
            return false;
        }
        file.expected = size;
    }
    // 文件流仍按单条回复的顺序映射到 2、4、6。当前映射禁止流号复用；
    // 其他映射需要真机协议证据，不能通过沿用残留字节猜测。
    for (std::size_t i = 0; i < files.size(); ++i) {
        const auto id = static_cast<uint32_t>((i + 1) * 2);
        if (verbose_) {
            LogDebug << std::format("Receiving file: {} bytes on stream {}", files[i]->file_size, id);
        }
        if (!receive_file(id, files[i]->file_size, deadline, files[i]->data, err)) {
            return false;
        }
    }
    return true;
}

bool Channel::take_message(xpc::Value& out, std::chrono::steady_clock::time_point deadline, std::string& err)
{
    for (auto& [stream, buf] : pending_) {
        for (;;) {
            xpc::Message m;
            std::size_t used = 0;
            std::string derr;
            const auto st = xpc::decode_message(buf, m, used, derr);
            if (st == xpc::Status::Malformed) {
                err = "Stream " + std::to_string(stream) + " contains malformed message: " + derr;
                return false;
            }
            if (st == xpc::Status::NeedMore) {
                // verbose 记录消息欠缺字节数，便于结合原始入流排查未完成的回复。
                // 仅凭本地缓冲不足不能断定对端是否已发完。
                if (verbose_) {
                    LogDebug << std::format("Stream {} message incomplete: {}", stream, derr);
                }
                break;
            }
            buf.erase(buf.begin(), buf.begin() + static_cast<std::ptrdiff_t>(used));
            if (stream >= 5 && stream % 2 == 1 && stream < next_outbound_file_stream_) {
                if (m.has_body || (m.flags & xpc::kFlagFileTxResponse) == 0) {
                    err = "Invalid RemoteXPC upload acknowledgement";
                    return false;
                }
                continue;
            }
            // 跳过握手 ACK 使用的空载荷和空字典；仅向调用方返回非空字典消息。
            if (m.has_body && m.body.is_dict() && !m.body.dict.empty()) {
                if (verbose_) {
                    LogDebug << std::format("=> stream {} id={} {}", stream, m.message_id, xpc::describe(m.body).substr(0, 300));
                }
                out = std::move(m.body);
                return materialize_files(out, deadline, err);
            }
        }
    }
    return false;
}

Channel::Wait Channel::wait(xpc::Value& out, int timeout_ms, std::string& err)
{
    err.clear();
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
    for (;;) {
        if (terminated_) {
            err = "Connection terminated";
            return Wait::Broken;
        }
        if (take_message(out, deadline, err)) {
            return Wait::Message;
        }
        if (!err.empty()) {
            terminated_ = true;
            return Wait::Broken; // 解析或文件接收失败。
        }
        const auto left = remaining_ms(deadline);
        if (left == 0) {
            err = "Timed out waiting for device reply";
            return Wait::Timeout;
        }
        if (terminated_) {
            err = "Connection terminated";
            return Wait::Broken;
        }
        if (!pump(static_cast<int>(left), err)) {
            // 已缓冲的数据不能把重置、资源超限或协议错误转成成功回复。
            err = "Disconnected while waiting for device reply: " + err;
            return Wait::Broken;
        }
    }
}

bool Channel::call(const xpc::Value& request, xpc::Value& reply, int timeout_ms, std::string& err)
{
    if (!send_request(request, true, err)) {
        return false;
    }
    return receive(reply, timeout_ms, err);
}

} // namespace maa::ios::remote
