#include "rt/RtpHevc.h"

#include <utility>

namespace maa::ios::rt
{
namespace
{

constexpr std::size_t kRtpHeaderLen = 12;
constexpr uint8_t kTypeAgg16 = 48;
constexpr uint8_t kTypeFragment = 49;

uint16_t be16(const uint8_t* p)
{
    return static_cast<uint16_t>(p[0] << 8 | p[1]);
}

uint32_t be32(const uint8_t* p)
{
    return uint32_t(p[0]) << 24 | uint32_t(p[1]) << 16 | uint32_t(p[2]) << 8 | p[3];
}

bool valid_nal_header(std::span<const uint8_t> nal)
{
    // 只交付未标记语法损坏的 NAL；temporal_id_plus1 必须非零。
    return nal.size() >= 2 && (nal[0] & 0x80) == 0 && (nal[1] & 7) != 0;
}

ReceivedNal complete_nal(std::span<const uint8_t> bytes, const PacketInfo& info, bool ends_access_unit)
{
    return { { bytes.begin(), bytes.end() }, info.timestamp, info.ssrc, info.sequence, info.sequence, ends_access_unit };
}

} // namespace

bool parse_rtp_header(std::span<const uint8_t> d, PacketInfo& out)
{
    if (d.size() < kRtpHeaderLen) {
        return false;
    }
    if ((d[0] >> 6) != 2) {
        return false; // 只认 RTP v2
    }
    std::size_t pos = kRtpHeaderLen;
    const std::size_t csrc = static_cast<std::size_t>(d[0] & 0x0F) * 4;
    if (csrc > 0) {
        if (d.size() < pos + csrc) {
            return false;
        }
        pos += csrc;
    }
    // 扩展长度以 32 位字计，不含扩展头自身的 4 字节；按声明长度跳过整个扩展。
    // 不再额外跳过固定长度的私有头。
    if ((d[0] & 0x10) != 0) {
        if (d.size() < pos + 4) {
            return false;
        }
        const std::size_t ext = (static_cast<std::size_t>(be16(d.data() + pos + 2)) + 1) * 4;
        if (d.size() < pos + ext) {
            return false;
        }
        pos += ext;
    }
    std::size_t end = d.size();
    if ((d[0] & 0x20) != 0) {
        const std::size_t padding = d.back();
        if (padding == 0 || padding > end - pos) {
            return false;
        }
        end -= padding;
    }
    if (end - pos < 2) {
        return false;
    }
    out = { (d[1] & 0x80) != 0, static_cast<uint8_t>(d[1] & 0x7F), be16(d.data() + 2), be32(d.data() + 4), be32(d.data() + 8), pos,
            end - pos };
    return true;
}

void HevcRtpDepacketizer::reset()
{
    if (!partial_.bytes.empty()) {
        ++stats_.dropped_fragments;
    }
    partial_ = { };
}

bool HevcRtpDepacketizer::push_nals(std::span<const uint8_t> datagram, std::vector<ReceivedNal>& out, std::string& err)
{
    PacketInfo info;
    if (!parse_rtp_header(datagram, info)) {
        ++stats_.malformed;
        // 无法识别其 PT/来源时，不让无关的坏包作废已收到的 FU。
        // 若缺失的是视频分片，下一条合法 FU 的序号连续性仍会拒绝残缺 NAL。
        err = "Not an RTP packet";
        return false;
    }
    err.clear();
    if (info.payload_type != video_pt_) {
        // 非视频 PT 整包跳过，不参与当前视频流的序号统计。
        ++stats_.other_payload;
        return true;
    }
    ++stats_.packets;
    // 由 RtpSeq 按模 2^16 跟踪序号，迟到包不改变当前最高序号。
    switch (seq_.observe(info.sequence)) {
    case RtpSeq::Verdict::kGap:
        ++stats_.seq_gaps;
        break;
    case RtpSeq::Verdict::kLate:
        ++stats_.reordered;
        // 当前没有媒体重排队列。旧包即使补齐接收统计中的缺口，也不能插回
        // 已处理的 NAL/AU 顺序；重复或迟到载荷不得关闭/替换正在组装的 FU。
        stats_.seq_lost = seq_.lost();
        return true;
    case RtpSeq::Verdict::kFirst:
    case RtpSeq::Verdict::kInOrder:
        break;
    }
    // 直接读取当前缺口数，保留窗口内迟到补齐带来的减少。
    stats_.seq_lost = seq_.lost();
    const auto payload = datagram.subspan(info.payload_offset, info.payload_size);
    if (!valid_nal_header(payload)) {
        ++stats_.malformed;
        reset();
        return true;
    }
    const uint8_t type = static_cast<uint8_t>((payload[0] >> 1) & 0x3F);
    auto body = payload.subspan(2);

    if (type == kTypeAgg16) {
        // AP 不能接续 FU。先校验所有长度和完整 NAL 头，禁止嵌套 AP/FU/PACI；
        // 若后部损坏，不将已验证的前缀交给调用方。
        reset();
        auto rest = body;
        std::size_t count = 0;
        while (!rest.empty()) {
            if (rest.size() < 2) {
                ++stats_.malformed;
                return true;
            }
            const std::size_t len = be16(rest.data());
            rest = rest.subspan(2);
            if (len < 2 || len > rest.size() || !valid_nal_header(rest.first(len)) || ((rest[0] >> 1) & 0x3F) >= kTypeAgg16) {
                ++stats_.malformed;
                return true;
            }
            rest = rest.subspan(len);
            ++count;
        }
        if (count == 0) {
            ++stats_.malformed;
            return true;
        }
        while (!body.empty()) {
            const std::size_t len = be16(body.data());
            body = body.subspan(2);
            out.push_back(complete_nal(body.first(len), info, info.marker && body.size() == len));
            body = body.subspan(len);
        }
        stats_.nals += count;
        return true;
    }

    if (type == kTypeFragment) {
        if (body.size() < 2) { // 至少 FU 头和一个载荷字节。
            ++stats_.malformed;
            reset();
            return true;
        }
        const uint8_t fu = body[0];
        const bool start = (fu & 0x80) != 0;
        const bool end = (fu & 0x40) != 0;
        const uint8_t inner_type = static_cast<uint8_t>(fu & 0x3F);
        if ((start && end) || inner_type >= kTypeAgg16 || (info.marker && !end)) {
            ++stats_.malformed;
            reset();
            return true;
        }
        // 保留起始片的 F、LayerId 和 TID，只有六位 type 来自 FU 头。
        const uint8_t header[2] = { static_cast<uint8_t>((payload[0] & 0x81) | (inner_type << 1)), payload[1] };
        body = body.subspan(1);
        const bool same_nal = !partial_.bytes.empty() && partial_.timestamp == info.timestamp && partial_.ssrc == info.ssrc
                              && partial_.bytes[0] == header[0] && partial_.bytes[1] == header[1];
        if (start) {
            reset();
            partial_ = { { header[0], header[1] }, info.timestamp, info.ssrc, info.sequence, info.sequence, false };
        }
        else if (partial_.bytes.empty()) {
            ++stats_.dropped_fragments;
            return true;
        }
        else if (!same_nal || info.sequence != static_cast<uint16_t>(partial_.last_sequence + 1U)) {
            reset();
            return true;
        }
        // 当前适配不含 DONL，FU 头之后的字节全部属于原始 NAL 载荷。
        partial_.bytes.insert(partial_.bytes.end(), body.begin(), body.end());
        partial_.last_sequence = info.sequence;
        if (end) {
            partial_.ends_access_unit = info.marker;
            out.push_back(std::move(partial_));
            partial_ = { };
            ++stats_.nals;
        }
        return true;
    }

    reset();
    if (type >= kTypeAgg16) { // 未实现 PACI，不将其包头当作原始 NAL。
        ++stats_.malformed;
        return true;
    }
    out.push_back(complete_nal(payload, info, info.marker));
    ++stats_.nals;
    return true;
}

} // namespace maa::ios::rt
