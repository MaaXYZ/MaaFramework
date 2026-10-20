#include "http2/Framing.h"
#include "util/Bytes.h"

#include <algorithm>
#include <cstdio>
#include <cstring>

namespace maa::ios::http2
{
namespace
{

constexpr uint32_t kStreamIdMask = 0x7FFFFFFF;

void put_u16(std::vector<uint8_t>& out, uint16_t v)
{
    out.push_back(static_cast<uint8_t>(v >> 8));
    out.push_back(static_cast<uint8_t>(v));
}

void put_u32(std::vector<uint8_t>& out, uint32_t v)
{
    out.push_back(static_cast<uint8_t>(v >> 24));
    out.push_back(static_cast<uint8_t>(v >> 16));
    out.push_back(static_cast<uint8_t>(v >> 8));
    out.push_back(static_cast<uint8_t>(v));
}

std::string hex32(uint32_t v)
{
    char buf[11];
    std::snprintf(buf, sizeof(buf), "0x%08x", v);
    return buf;
}

} // namespace

const char* frame_type_name(uint8_t type)
{
    switch (type) {
    case kData:
        return "DATA";
    case kHeaders:
        return "HEADERS";
    case kPriority:
        return "PRIORITY";
    case kRstStream:
        return "RST_STREAM";
    case kSettings:
        return "SETTINGS";
    case kPushPromise:
        return "PUSH_PROMISE";
    case kPing:
        return "PING";
    case kGoAway:
        return "GOAWAY";
    case kWindowUpdate:
        return "WINDOW_UPDATE";
    case kContinuation:
        return "CONTINUATION";
    default:
        return "UNKNOWN";
    }
}

const char* error_code_name(uint32_t code)
{
    switch (code) {
    case kNoError:
        return "NO_ERROR";
    case kProtocolError:
        return "PROTOCOL_ERROR";
    case kInternalError:
        return "INTERNAL_ERROR";
    case kFlowControlError:
        return "FLOW_CONTROL_ERROR";
    case kFrameSizeError:
        return "FRAME_SIZE_ERROR";
    case kCompressionError:
        return "COMPRESSION_ERROR";
    default:
        return "OTHER";
    }
}

std::vector<uint8_t> serialize(uint8_t type, uint8_t flags, uint32_t stream_id, std::span<const uint8_t> payload)
{
    if (payload.size() > kProtocolMaxFrameSize) {
        return { }; // 长度必须能完整写入帧头的 24 位字段。
    }
    std::vector<uint8_t> out;
    out.reserve(kFrameHeaderSize + payload.size());
    const uint32_t len = static_cast<uint32_t>(payload.size());
    out.push_back(static_cast<uint8_t>(len >> 16));
    out.push_back(static_cast<uint8_t>(len >> 8));
    out.push_back(static_cast<uint8_t>(len));
    out.push_back(type);
    out.push_back(flags);
    put_u32(out, stream_id & kStreamIdMask);
    out.insert(out.end(), payload.begin(), payload.end());
    return out;
}

std::vector<uint8_t> settings_frame(const std::vector<std::pair<uint16_t, uint32_t>>& settings)
{
    std::vector<uint8_t> body;
    body.reserve(settings.size() * 6);
    for (const auto& [id, value] : settings) {
        put_u16(body, id);
        put_u32(body, value);
    }
    return serialize(kSettings, 0, 0, body);
}

std::vector<uint8_t> settings_ack_frame()
{
    return serialize(kSettings, kFlagAck, 0, { });
}

std::vector<uint8_t> window_update_frame(uint32_t stream_id, uint32_t increment)
{
    std::vector<uint8_t> body;
    put_u32(body, increment);
    return serialize(kWindowUpdate, 0, stream_id, body);
}

std::vector<uint8_t> headers_frame(uint32_t stream_id)
{
    // RemoteXPC 通过空 header block 和 END_HEADERS 建立流。
    return serialize(kHeaders, kFlagEndHeaders, stream_id, { });
}

std::vector<uint8_t> data_frame(uint32_t stream_id, std::span<const uint8_t> payload, uint8_t extra_flags)
{
    return serialize(kData, extra_flags, stream_id, payload);
}

std::vector<uint8_t> ping_frame(uint64_t opaque_data, bool ack)
{
    std::vector<uint8_t> body;
    for (int i = 7; i >= 0; --i) {
        body.push_back(static_cast<uint8_t>(opaque_data >> (8 * i)));
    }
    return serialize(kPing, ack ? kFlagAck : 0, 0, body);
}

Status parse_frame(std::span<const uint8_t> buf, Frame& out, std::size_t& consumed, std::string& err)
{
    consumed = 0;
    if (buf.size() < kFrameHeaderSize) {
        err = "Frame header shorter than 9 bytes";
        return Status::NeedMore;
    }
    const std::size_t len =
        static_cast<std::size_t>(buf[0]) << 16 | static_cast<std::size_t>(buf[1]) << 8 | static_cast<std::size_t>(buf[2]);
    if (len > kMaxFrameSize) {
        err = std::string("Frame length too large: ") + std::to_string(len);
        return Status::Malformed;
    }
    const std::size_t total = kFrameHeaderSize + len;
    if (buf.size() < total) {
        err = "Missing " + std::to_string(total - buf.size()) + " bytes";
        return Status::NeedMore;
    }
    out.type = buf[3];
    out.flags = buf[4];
    out.stream_id = util::read_be32(buf.data() + 5) & kStreamIdMask;
    out.payload.assign(buf.begin() + static_cast<std::ptrdiff_t>(kFrameHeaderSize), buf.begin() + static_cast<std::ptrdiff_t>(total));
    consumed = total;
    return Status::Ok;
}

bool data_payload(const Frame& f, std::span<const uint8_t>& out, std::string& err)
{
    std::span<const uint8_t> body(f.payload.data(), f.payload.size());
    // DATA 仅定义 END_STREAM 和 PADDED；其余标志按 RFC 7540 §4.1 忽略。
    // 0x20 的优先级字段属于 HEADERS，在 DATA 中不能据此移除载荷字节。
    if ((f.flags & kFlagPadded) != 0) {
        if (body.empty()) {
            err = "PADDED DATA missing Pad Length byte";
            return false;
        }
        // Pad Length 为首字节，填充位于载荷尾部。
        const std::size_t padding = body[0];
        body = body.subspan(1);
        if (padding > body.size()) {
            err = "Pad Length exceeds available payload: " + std::to_string(padding);
            return false;
        }
        body = body.subspan(0, body.size() - padding);
    }
    out = body;
    return true;
}

bool parse_settings(const Frame& f, std::vector<std::pair<uint16_t, uint32_t>>& out, bool& ack, std::string& err)
{
    ack = (f.flags & kFlagAck) != 0;
    out.clear();
    if (f.stream_id != 0) {
        err = "SETTINGS must use stream 0";
        return false;
    }
    if (ack) {
        if (!f.payload.empty()) {
            err = "SETTINGS ACK payload must be empty";
            return false;
        }
        return true;
    }
    if (f.payload.size() % 6 != 0) {
        err = "SETTINGS payload length is not a multiple of 6: " + std::to_string(f.payload.size());
        return false;
    }
    for (std::size_t i = 0; i + 6 <= f.payload.size(); i += 6) {
        const auto id = util::read_be16(f.payload.data() + i);
        const auto value = util::read_be32(f.payload.data() + i + 2);
        if (id == kSettingInitialWindowSize && value > kMaxWindowSize) {
            err = "SETTINGS_INITIAL_WINDOW_SIZE exceeds 2^31 - 1";
            return false;
        }
        if (id == kSettingMaxFrameSize && (value < kDefaultMaxFrameSize || value > kProtocolMaxFrameSize)) {
            err = "SETTINGS_MAX_FRAME_SIZE is outside 16384..16777215";
            return false;
        }
        out.emplace_back(id, value);
    }
    return true;
}

bool parse_window_update(const Frame& f, uint32_t& increment, std::string& err)
{
    if (f.payload.size() != 4) {
        err = "WINDOW_UPDATE payload must be 4 bytes; got " + std::to_string(f.payload.size());
        return false;
    }
    increment = util::read_be32(f.payload.data()) & kStreamIdMask;
    if (increment == 0) {
        err = "WINDOW_UPDATE increment must be nonzero";
        return false;
    }
    return true;
}

bool parse_goaway(const Frame& f, GoAway& out, std::string& err)
{
    if (f.payload.size() < 8) {
        err = "GOAWAY payload shorter than 8 bytes";
        return false;
    }
    out.last_stream_id = util::read_be32(f.payload.data()) & kStreamIdMask;
    out.error_code = util::read_be32(f.payload.data() + 4);
    out.debug_data.assign(reinterpret_cast<const char*>(f.payload.data()) + 8, f.payload.size() - 8);
    // 调试串是不可信输入，只留可打印字符，别让它把终端控制序列打出来。
    std::erase_if(out.debug_data, [](char c) {
        const auto u = static_cast<unsigned char>(c);
        return u < 0x20 || u == 0x7f;
    });
    return true;
}

bool parse_rst_stream(const Frame& f, uint32_t& error_code, std::string& err)
{
    if (f.payload.size() != 4) {
        err = "RST_STREAM payload must be 4 bytes; got " + std::to_string(f.payload.size());
        return false;
    }
    error_code = util::read_be32(f.payload.data());
    return true;
}

std::string describe(const Frame& f)
{
    char head[96];
    std::snprintf(
        head,
        sizeof(head),
        "%-13s stream=%u flags=0x%02x len=%zu",
        frame_type_name(f.type),
        f.stream_id,
        f.flags,
        f.payload.size());
    std::string s = head;
    // 只有会说话的那几种帧才值得展开：设备报错时人话写在 GOAWAY 的调试串里，
    // 而码点藏在 SETTINGS 里，这两处正是排查握手为什么不通过时唯一要看的东西。
    if (f.type == kGoAway) {
        GoAway g;
        std::string e;
        if (parse_goaway(f, g, e)) {
            s += " ";
            s += error_code_name(g.error_code);
            s += " last=";
            s += std::to_string(g.last_stream_id);
            if (!g.debug_data.empty()) {
                s += " \"";
                s += g.debug_data;
                s += '"';
            }
        }
    }
    else if (f.type == kRstStream) {
        uint32_t code = 0;
        std::string e;
        if (parse_rst_stream(f, code, e)) {
            s += " ";
            s += error_code_name(code);
        }
    }
    else if (f.type == kSettings && (f.flags & kFlagAck) == 0) {
        std::vector<std::pair<uint16_t, uint32_t>> values;
        bool ack = false;
        std::string e;
        if (parse_settings(f, values, ack, e)) {
            for (const auto& [id, value] : values) {
                s += " ";
                s += hex32(id);
                s += '=';
                s += std::to_string(value);
            }
        }
    }
    return s;
}

} // namespace maa::ios::http2
