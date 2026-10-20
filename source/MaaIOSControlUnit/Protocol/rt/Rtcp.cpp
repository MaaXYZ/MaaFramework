#include "rt/Rtcp.h"

namespace maa::ios::rt
{
namespace
{

uint32_t read32(std::span<const uint8_t> bytes, std::size_t offset)
{
    return (uint32_t(bytes[offset]) << 24) | (uint32_t(bytes[offset + 1]) << 16) | (uint32_t(bytes[offset + 2]) << 8)
           | uint32_t(bytes[offset + 3]);
}

void put16(std::vector<uint8_t>& v, uint16_t x)
{
    v.push_back(static_cast<uint8_t>(x >> 8));
    v.push_back(static_cast<uint8_t>(x));
}

void put32(std::vector<uint8_t>& v, uint32_t x)
{
    v.push_back(static_cast<uint8_t>(x >> 24));
    v.push_back(static_cast<uint8_t>(x >> 16));
    v.push_back(static_cast<uint8_t>(x >> 8));
    v.push_back(static_cast<uint8_t>(x));
}

} // namespace

bool parse_sender_reports(std::span<const uint8_t> datagram, std::vector<SenderReport>& reports)
{
    if (datagram.empty()) {
        return false;
    }
    std::vector<SenderReport> parsed;
    while (!datagram.empty()) {
        if (datagram.size() < 4 || (datagram[0] >> 6) != 2 || datagram[1] < 192 || datagram[1] > 223) {
            return false;
        }
        const std::size_t words = (std::size_t(datagram[2]) << 8) | datagram[3];
        const std::size_t size = (words + 1) * 4;
        if (size > datagram.size()) {
            return false;
        }
        auto packet = datagram.first(size);
        if ((packet[0] & 0x20) != 0) {
            // RFC 3550：填充只能位于末包，末字节包含填充量，且须为四的倍数。
            const std::size_t padding = packet.back();
            if (size != datagram.size() || padding == 0 || padding % 4 != 0 || padding > size - 4) {
                return false;
            }
            packet = packet.first(size - padding);
        }
        const std::size_t count = packet[0] & 0x1f;
        if (packet[1] == 200 || packet[1] == 201) {
            const std::size_t base = packet[1] == 200 ? 28 : 8;
            if (packet.size() < base + count * 24) {
                return false;
            }
            if (packet[1] == 200) {
                parsed.push_back(
                    { read32(packet, 4),
                      read32(packet, 8),
                      read32(packet, 12),
                      read32(packet, 16),
                      read32(packet, 20),
                      read32(packet, 24) });
            }
        }
        datagram = datagram.subspan(size);
    }
    reports.swap(parsed);
    return true;
}

std::vector<uint8_t> build_rr(uint32_t sender_ssrc, uint32_t media_ssrc, uint32_t ext_high)
{
    std::vector<uint8_t> v;
    v.push_back(0x81); // V=2, RC=1
    v.push_back(201);  // PT = RR
    put16(v, 7);       // 长度以 4 字节为单位，不含第一个字
    put32(v, sender_ssrc);
    put32(v, media_ssrc);
    put32(v, 0);        // 丢包比例 1 字节 + 累计丢包 3 字节，当前未填接收统计
    put32(v, ext_high); // 扩展最高序号
    put32(v, 0);        // 抖动
    put32(v, 0);        // LSR，当前未记录对端 SR 时间
    put32(v, 0);        // DLSR，当前未计算接收 SR 后的延迟
    return v;
}

bool is_rtcp_sr(std::span<const uint8_t> datagram)
{
    return datagram.size() >= 28 && datagram[0] == 0x81 && datagram[1] == 0xc8;
}

std::vector<uint8_t> build_pli(uint32_t sender_ssrc, uint32_t media_ssrc)
{
    std::vector<uint8_t> v;
    v.push_back(0x81); // V=2, FMT=1（PLI）
    v.push_back(206);  // PT = PSFB
    put16(v, 2);       // 头之后两个字：发送者 SSRC + 媒体 SSRC
    put32(v, sender_ssrc);
    put32(v, media_ssrc);
    return v;
}

} // namespace maa::ios::rt
