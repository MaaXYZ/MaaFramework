// RTP 拆包自检。全部用手工构造的包，不打真机——这样每一处偏移错了都会立刻
// 变成一条明确的失败，而不是"画面有点糊"。
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <span>
#include <string>
#include <vector>

#include "bitstream/AnnexB.h"
#include "rt/Rtcp.h"
#include "rt/RtpHevc.h"
#include "rt/RtpSeq.h"

namespace
{

int Failures = 0;

void check(bool ok, const std::string& what)
{
    std::printf("  [%s] %s\n", ok ? "PASS" : "FAIL", what.c_str());
    if (!ok) {
        ++Failures;
    }
}

using maa::ios::rt::HevcRtpDepacketizer;
using maa::ios::rt::ReceivedNal;
using maa::ios::rt::RtpSeq;

/// 一个 RTP 包，按真机的样子构造：12 字节头（X=1）+ 8 字节扩展头 + HEVC 载荷。
/// 扩展头的形状是从真机包上抄的（profile 0x9011、长度 1 个 32 位字），不是编的——
/// 上一版测试用了"X=0 + 8 字节私有子头"，正好和被测代码里多跳 8 字节的错误自洽，
/// 于是测试全绿、真机全废。
std::vector<uint8_t>
    packet(uint16_t seq, uint32_t ts, bool marker, const std::vector<uint8_t>& payload, uint8_t pt = 100, uint32_t ssrc = 0xDEADBEEF)
{
    std::vector<uint8_t> p;
    p.push_back(0x90); // V=2, P=0, X=1, CC=0
    p.push_back(pt | (marker ? 0x80 : 0));
    p.push_back(static_cast<uint8_t>(seq >> 8));
    p.push_back(static_cast<uint8_t>(seq));
    for (int i = 3; i >= 0; --i) {
        p.push_back(static_cast<uint8_t>(ts >> (8 * i)));
    }
    for (int i = 0; i < 4; ++i) {
        p.push_back(static_cast<uint8_t>(ssrc >> (8 * (3 - i))));
    }
    p.push_back(0x90);
    p.push_back(0x11);                                     // profile
    p.push_back(0x00);
    p.push_back(0x01);                                     // 长度：1 个 32 位字
    for (int i = 0; i < 4; ++i) {
        p.push_back(static_cast<uint8_t>(0x00107954 + i)); // 扩展内容，我们不读
    }
    p.insert(p.end(), payload.begin(), payload.end());
    return p;
}

/// 一个 HEVC NAL 头。
std::vector<uint8_t> nal_header(uint8_t type, uint8_t layer = 0, uint8_t tid = 1)
{
    return { static_cast<uint8_t>(((type << 1) & 0x7E) | ((layer >> 5) & 1)), static_cast<uint8_t>((layer << 3) | (tid & 7)) };
}

std::vector<uint8_t> aggregation(const std::vector<std::vector<uint8_t>>& nals)
{
    std::vector<uint8_t> out = nal_header(48);
    for (const auto& n : nals) {
        out.push_back(static_cast<uint8_t>(n.size() >> 8));
        out.push_back(static_cast<uint8_t>(n.size()));
        out.insert(out.end(), n.begin(), n.end());
    }
    return out;
}

std::vector<uint8_t> fragment(uint8_t type, const std::vector<uint8_t>& body, bool start, bool end, uint8_t layer = 0, uint8_t tid = 1)
{
    std::vector<uint8_t> out = nal_header(49, layer, tid);
    out.push_back(static_cast<uint8_t>((start ? 0x80 : 0) | (end ? 0x40 : 0) | type));
    out.insert(out.end(), body.begin(), body.end());
    return out;
}

std::vector<uint8_t> single(uint8_t type, const std::vector<uint8_t>& body)
{
    auto out = nal_header(type);
    out.insert(out.end(), body.begin(), body.end());
    return out;
}

uint8_t nal_type(const std::vector<uint8_t>& nal)
{
    return nal.empty() ? 0xFF : static_cast<uint8_t>((nal[0] >> 1) & 0x3F);
}

void test_aggregation()
{
    std::printf("\n== 聚合包 ==\n");
    std::vector<uint8_t> vps = nal_header(32);
    for (int i = 0; i < 22; ++i) {
        vps.push_back(static_cast<uint8_t>(i));
    }
    std::vector<uint8_t> sps = nal_header(33);
    for (int i = 0; i < 61; ++i) {
        sps.push_back(static_cast<uint8_t>(0x40 + i));
    }
    auto p = packet(1, 0, false, aggregation({ vps, sps }));
    HevcRtpDepacketizer d;
    std::string err;
    std::vector<ReceivedNal> out;
    check(d.push_nals(p, out, err), "聚合包可解: " + err);
    const auto& nals = out;
    check(nals.size() == 2, "出两个 NAL");
    if (nals.size() == 2) {
        check(nal_type(nals[0].bytes) == 32 && nals[0].bytes.size() == 24, "VPS 类型与长度");
        check(nal_type(nals[1].bytes) == 33 && nals[1].bytes == sps, "SPS 内容逐字节一致");
    }
}

void test_fragmentation()
{
    std::printf("\n== 分片 ==\n");
    std::vector<uint8_t> body;
    for (int i = 0; i < 300; ++i) {
        body.push_back(static_cast<uint8_t>(i * 3 + 1));
    }
    HevcRtpDepacketizer d;
    std::string err;
    std::vector<ReceivedNal> out;
    const auto f1 = fragment(20, { body.begin(), body.begin() + 120 }, true, false);
    const auto f2 = fragment(20, { body.begin() + 120, body.begin() + 240 }, false, false);
    const auto f3 = fragment(20, { body.begin() + 240, body.end() }, false, true);
    d.push_nals(packet(10, 5000, false, f1), out, err);
    check(d.mid_fragment(), "起始分片后处于半成品状态");
    d.push_nals(packet(11, 5000, false, f2), out, err);
    d.push_nals(packet(12, 5000, true, f3), out, err);
    check(!d.mid_fragment(), "结尾分片后清空");
    const auto& nals = out;
    check(nals.size() == 1, "三片拼成一个 NAL");
    if (nals.size() == 1) {
        check(nal_type(nals[0].bytes) == 20, "TU 还原成 NAL 类型");
        check(nals[0].bytes.size() == 2 + 300, "长度 = NAL 头 + 全部分片数据");
        // 这一条专门钉住"没有 DONL"：起始分片 FU 头之后的两字节就是码流本身。
        // 若按 RFC 7798 把它当 DONL 跳掉，这里会少 2 字节且整体错位。
        check(
            std::equal(body.begin(), body.end(), nals[0].bytes.begin() + 2),
            "起始分片 FU 头后的字节被当码流保留（这条流没有 DONL 字段）");
    }
}

void test_lost_fragments()
{
    std::printf("\n== 丢分片 ==\n");
    HevcRtpDepacketizer d;
    std::string err;
    std::vector<ReceivedNal> out;
    // 中间片先到（起始片丢了）：整个 NAL 拼不出来，必须丢，不能交半截给解码器
    d.push_nals(packet(1, 100, false, fragment(1, { 0xAA, 0xBB }, false, false)), out, err);
    check(out.empty(), "缺起始片的中间片不产出 NAL");
    d.push_nals(packet(2, 100, true, fragment(1, { 0xCC }, false, true)), out, err);
    check(out.empty(), "缺起始片的结尾片也不产出 NAL");
    check(d.stats().dropped_fragments == 2, "两次都记进 dropped_fragments: " + std::to_string(d.stats().dropped_fragments));

    // 起始片来了两次（上一帧的结尾片丢了）：旧的半成品要作废，不能串帧
    out.clear();
    d.push_nals(packet(3, 200, false, fragment(1, { 0x11, 0x11 }, true, false)), out, err);
    d.push_nals(packet(4, 300, false, fragment(1, { 0x22, 0x22 }, true, false)), out, err);
    d.push_nals(packet(5, 300, true, fragment(1, { 0x33, 0x33 }, false, true)), out, err);
    const auto& nals = out;
    check(nals.size() == 1 && nals[0].bytes.size() == 2 + 4, "新起始片丢弃上一帧的半成品");
    check(nals.size() == 1 && nals[0].bytes[2] == 0x22, "产出的是第二个 NAL 的内容");
    d.reset();
    check(!d.mid_fragment(), "reset 清空半成品");
}

void test_single_and_offsets()
{
    std::printf("\n== 单一 NAL 与头偏移 ==\n");
    HevcRtpDepacketizer d;
    std::string err;
    std::vector<ReceivedNal> out;
    d.push_nals(packet(7, 900, true, single(1, { 0xA4, 0x01, 0x02 })), out, err);
    const auto& nals = out;
    check(nals.size() == 1 && nal_type(nals[0].bytes) == 1 && nals[0].bytes.size() == 5, "单一 NAL 直通");

    // 带 CSRC 与扩展头的包：偏移算错就会把 CSRC/扩展当载荷，解出垃圾 NAL 类型
    auto base = single(1, { 0xA4 });
    std::vector<uint8_t> with_csrc;
    with_csrc.push_back(0x92); // V=2, X=1, CC=2 -> 8 字节 CSRC 再接 8 字节扩展头
    with_csrc.push_back(100);
    with_csrc.push_back(0);
    with_csrc.push_back(8);       // 与前一包连续，单独验证 CSRC/扩展偏移。
    for (int i = 0; i < 8; ++i) { // 补齐到 12 字节 RTP 头
        with_csrc.push_back(0);
    }
    for (int i = 0; i < 8; ++i) { // CC=2 的 CSRC 列表
        with_csrc.push_back(static_cast<uint8_t>(0xC0 + i));
    }
    with_csrc.push_back(0x90);
    with_csrc.push_back(0x11); // 扩展头 profile
    with_csrc.push_back(0x00);
    with_csrc.push_back(0x01); // 长度 1 个 32 位字 -> 扩展共 8 字节
    for (int i = 0; i < 4; ++i) {
        with_csrc.push_back(0x5A);
    }
    with_csrc.insert(with_csrc.end(), base.begin(), base.end());
    std::vector<ReceivedNal> out2;
    check(d.push_nals(with_csrc, out2, err), "带 CSRC 的包可解");
    const auto& n2 = out2;
    check(n2.size() == 1 && nal_type(n2[0].bytes) == 1, "CSRC 被正确跳过");

    std::vector<uint8_t> with_ext;
    with_ext.push_back(0x90); // V=2, X=1, CC=0
    with_ext.push_back(100);
    with_ext.push_back(0);
    with_ext.push_back(9);
    for (int i = 0; i < 8; ++i) {
        with_ext.push_back(0);
    }
    with_ext.push_back(0xBE);
    with_ext.push_back(0xDE);
    with_ext.push_back(0x00);
    with_ext.push_back(0x02); // 扩展长度 2 个 32 位字 -> 共 12 字节
    for (int i = 0; i < 8; ++i) {
        with_ext.push_back(0x77);
    }
    // 这个包自己带一个 2 字的扩展，覆盖"按扩展自身长度跳"的分支
    with_ext.insert(with_ext.end(), base.begin(), base.end());
    std::vector<ReceivedNal> out3;
    check(d.push_nals(with_ext, out3, err), "带扩展头的包可解");
    const auto& n3 = out3;
    check(n3.size() == 1 && nal_type(n3[0].bytes) == 1, "扩展头按自身长度跳过");

    // 非 RTP / 太短
    std::vector<uint8_t> junk = { 1, 2, 3 };
    check(!d.push_nals(junk, out3, err), "3 字节的垃圾被拒");
    std::vector<uint8_t> v1(30, 0x40);
    check(!d.push_nals(v1, out3, err), "RTP 版本不是 2 被拒");
}

void test_payload_type_filter()
{
    std::printf("\n== 只收视频 PT ==\n");
    // RTCP 与视频同端口到达（真机实测 PT=72）。不过滤就会被当 HEVC 载荷解出
    // 假 NAL 混进码流，而这条流不周期发 IDR，一个假 NAL 就把参考链永久打断。
    std::vector<uint8_t> rtcp;
    rtcp.push_back(0x82);
    rtcp.push_back(72);
    rtcp.insert(rtcp.end(), 20, 0x00);
    HevcRtpDepacketizer d;
    std::string err;
    std::vector<ReceivedNal> out;
    d.push_nals(packet(1, 0, false, aggregation({ nal_header(32) })), out, err);
    const auto after_video = out.size();
    d.push_nals(rtcp, out, err);
    check(out.size() == after_video, "RTCP 包不产出 NAL");
    check(d.stats().other_payload == 1 && d.stats().packets == 1, "RTCP 记成 other_payload，不计入视频包数与序号");
    // 序号也不能被它带跳：再来一个正常的下一号包，不该算丢包
    d.push_nals(packet(2, 100, true, single(1, { 0xA4 })), out, err);
    check(d.stats().seq_gaps == 0, "夹了 RTCP 之后视频序号仍然连续");

    // 配成别的 PT（比如协商到 96）时，100 的包不该被解
    HevcRtpDepacketizer other(96);
    std::vector<ReceivedNal> none;
    other.push_nals(packet(1, 0, false, single(1, { 0xA4 })), none, err);
    check(none.empty() && other.stats().other_payload == 1, "PT 不匹配就整包跳过");
}

void test_feeds_annexb_parser()
{
    std::printf("\n== 与 AU 切分器对接 ==\n");
    // 将完整 NAL 编码为 Annex-B，验证字节流解析器对单包与多包图像的 AU 切分。
    HevcRtpDepacketizer d;
    std::vector<std::size_t> au_sizes;
    std::size_t keyframes = 0;
    maa::ios::AnnexBParser parser([&](std::vector<maa::ios::Nal>&& au, bool key) {
        au_sizes.push_back(au.size());
        if (key) {
            ++keyframes;
        }
    });
    std::string err;
    std::vector<ReceivedNal> out;
    std::vector<uint8_t> vps_nal = nal_header(32);
    vps_nal.push_back(0x01);
    std::vector<uint8_t> sps_nal = nal_header(33);
    sps_nal.push_back(0x02);
    d.push_nals(packet(1, 0, false, aggregation({ vps_nal, sps_nal })), out, err);
    d.push_nals(packet(2, 0, false, fragment(20, { 0x80, 0x01 }, true, false)), out, err);
    d.push_nals(packet(3, 0, true, fragment(20, { 0x02, 0x03 }, false, true)), out, err);
    for (const auto& nal : out) {
        const uint8_t start_code[] = { 0, 0, 0, 1 };
        check(
            parser.feed(start_code, sizeof(start_code)) && parser.feed(nal.bytes.data(), nal.bytes.size()),
            "完整 NAL 可转为 Annex-B 字节流");
    }
    out.clear();
    d.push_nals(packet(4, 400, true, single(1, { 0xA4, 0x11 })), out, err);
    for (const auto& nal : out) {
        const uint8_t start_code[] = { 0, 0, 0, 1 };
        check(
            parser.feed(start_code, sizeof(start_code)) && parser.feed(nal.bytes.data(), nal.bytes.size()),
            "完整 NAL 可转为 Annex-B 字节流");
    }
    parser.flush();
    check(au_sizes.size() == 2, "切出 2 个 AU: " + std::to_string(au_sizes.size()));
    check(keyframes == 1, "其中 1 个是关键帧");
    check(!au_sizes.empty() && au_sizes[0] == 3, "关键帧 AU = VPS + SPS + IDR");
}

void test_complete_nal_metadata()
{
    std::printf("\n== Complete NAL metadata ==\n");
    using maa::ios::rt::ReceivedNal;
    HevcRtpDepacketizer d;
    std::string err;
    const ReceivedNal sentinel = { { 0x40, 1, 0x91 }, 3, 4, 5, 5, false };
    std::vector<ReceivedNal> out { sentinel };
    const auto original = single(1, { 0x80, 0, 0, 3, 1, 0x55 });
    check(d.push_nals(packet(65000, 0xfedcba98U, true, original, 100, 0x12345678U), out, err), "complete single NAL is accepted");
    check(
        out.size() == 2 && out[0] == sentinel && out[1] == ReceivedNal { original, 0xfedcba98U, 0x12345678U, 65000, 65000, true },
        "single NAL preserves bytes, unsigned timestamp, SSRC, sequence and marker; output appends");
    d.push_nals(packet(65001, 90, false, original, 100, 0x12345678U), out, err);
    check(
        out.size() == 3 && out.back().timestamp == 90 && !out.back().ends_access_unit,
        "raw timestamp is preserved without expansion, clamping or epoch inference");

    const auto vps = single(32, { 0x11, 0x22 });
    const auto idr = single(20, { 0x80, 0x33 });
    out.clear();
    d.push_nals(packet(65002, 120, true, aggregation({ vps, idr }), 100, 0x12345678U), out, err);
    check(
        out.size() == 2 && out[0] == ReceivedNal { vps, 120, 0x12345678U, 65002, 65002, false }
            && out[1] == ReceivedNal { idr, 120, 0x12345678U, 65002, 65002, true },
        "AP retains each NAL and attaches the packet marker only to its final NAL");
    out.clear();
    d.push_nals(packet(65003, 140, false, aggregation({ vps })), out, err);
    check(out.size() == 1 && out[0].bytes == vps && !out[0].ends_access_unit, "one-NAL AP is accepted");

    HevcRtpDepacketizer fu;
    out.clear();
    fu.push_nals(packet(65535, 0xf1234567U, false, fragment(20, { 0, 0 }, true, false, 37, 4), 100, 0x87654321U), out, err);
    check(
        out.empty() && fu.mid_fragment() && fu.fragment_timestamp() == 0xf1234567U,
        "FU start retains metadata but does not publish an incomplete NAL");
    fu.push_nals(packet(0, 0xf1234567U, false, fragment(20, { 3, 7 }, false, false, 37, 4), 100, 0x87654321U), out, err);
    fu.push_nals(packet(1, 0xf1234567U, true, fragment(20, { 8 }, false, true, 37, 4), 100, 0x87654321U), out, err);
    auto expected = nal_header(20, 37, 4);
    expected.insert(expected.end(), { 0, 0, 3, 7, 8 });
    check(
        out.size() == 1 && out[0] == ReceivedNal { expected, 0xf1234567U, 0x87654321U, 65535, 1, true } && !fu.mid_fragment(),
        "FU restores both layer bits and temporal ID, retains raw bytes and crosses sequence wrap");
}

void test_aggregation_atomicity()
{
    std::printf("\n== AP validation before output ==\n");
    const auto first = single(32, { 0x91 });
    const auto second = single(33, { 0x92, 0x93 });
    std::vector<std::vector<uint8_t>> malformed;
    auto truncated = aggregation({ first, second });
    truncated.pop_back();
    malformed.push_back(truncated);
    auto trailing = aggregation({ first, second });
    trailing.push_back(0xaa);
    malformed.push_back(trailing);
    auto zero_length = aggregation({ first });
    zero_length.insert(zero_length.end(), { 0, 0 });
    malformed.push_back(zero_length);
    auto short_header = aggregation({ first });
    short_header.insert(short_header.end(), { 0, 1, 2 });
    malformed.push_back(short_header);
    malformed.push_back(aggregation({ first, aggregation({ second }) }));
    malformed.push_back(aggregation({ first, fragment(1, { 0x80 }, true, false) }));
    malformed.push_back(aggregation({ first, single(50, { 0x80 }) }));
    auto forbidden = second;
    forbidden[0] |= 0x80;
    malformed.push_back(aggregation({ first, forbidden }));
    auto no_temporal_id = second;
    no_temporal_id[1] &= 0xf8;
    malformed.push_back(aggregation({ first, no_temporal_id }));
    malformed.push_back(nal_header(48));

    for (std::size_t i = 0; i < malformed.size(); ++i) {
        HevcRtpDepacketizer typed;
        std::string err;
        const maa::ios::rt::ReceivedNal sentinel = { first, 10, 11, 12, 12, false };
        std::vector<maa::ios::rt::ReceivedNal> nals { sentinel };
        const auto bad = packet(1, 500, true, malformed[i]);
        check(
            typed.push_nals(bad, nals, err) && nals.size() == 1 && nals[0] == sentinel && typed.stats().nals == 0
                && typed.stats().malformed == 1,
            "malformed AP retains typed output without publishing its valid prefix: " + std::to_string(i));
    }
}

void test_fragment_integrity()
{
    std::printf("\n== FU identity and continuity ==\n");
    const auto start = packet(10, 500, false, fragment(20, { 0x80, 1 }, true, false, 37, 4));
    const auto middle = packet(11, 500, false, fragment(20, { 2, 3 }, false, false, 37, 4));
    const auto finish = packet(12, 500, true, fragment(20, { 4, 5 }, false, true, 37, 4));
    const std::vector<std::vector<uint8_t>> mismatched = {
        packet(12, 500, false, fragment(20, { 2, 3 }, false, false, 37, 4)),
        packet(11, 501, false, fragment(20, { 2, 3 }, false, false, 37, 4)),
        packet(11, 500, false, fragment(20, { 2, 3 }, false, false, 37, 4), 100, 0x12345678),
        packet(11, 500, false, fragment(21, { 2, 3 }, false, false, 37, 4)),
        packet(11, 500, false, fragment(20, { 2, 3 }, false, false, 36, 4)),
        packet(11, 500, false, fragment(20, { 2, 3 }, false, false, 5, 4)),
        packet(11, 500, false, fragment(20, { 2, 3 }, false, false, 37, 3)),
        packet(11, 500, false, fragment(20, { }, false, false, 37, 4)),
        packet(11, 500, false, fragment(20, { 2, 3 }, true, true, 37, 4)),
        packet(11, 500, true, fragment(20, { 2, 3 }, false, false, 37, 4)),
        packet(11, 500, false, fragment(49, { 2, 3 }, false, false, 37, 4)),
    };
    for (std::size_t i = 0; i < mismatched.size(); ++i) {
        HevcRtpDepacketizer d;
        std::string err;
        std::vector<maa::ios::rt::ReceivedNal> out;
        d.push_nals(start, out, err);
        d.push_nals(mismatched[i], out, err);
        d.push_nals(finish, out, err);
        check(
            out.empty() && !d.mid_fragment() && d.stats().dropped_fragments > 0,
            "FU mismatch discards the partial NAL and later tail: " + std::to_string(i));
        // 无需重置序号统计即可接收下一条合法 NAL；错误不能留下旧载荷。
        d.push_nals(packet(20, 600, false, fragment(1, { 0x80, 6 }, true, false)), out, err);
        d.push_nals(packet(21, 600, false, fragment(1, { 7 }, false, true)), out, err);
        check(
            out.size() == 1 && out[0].bytes == single(1, { 0x80, 6, 7 }) && out[0].timestamp == 600 && out[0].first_sequence == 20
                && out[0].last_sequence == 21 && !out[0].ends_access_unit,
            "next complete FU has only its own bytes and metadata: " + std::to_string(i));
    }

    HevcRtpDepacketizer duplicates;
    std::vector<maa::ios::rt::ReceivedNal> out;
    std::string err;
    for (const auto& p : { start, start, middle, middle, finish, finish }) {
        duplicates.push_nals(p, out, err);
    }
    auto complete = nal_header(20, 37, 4);
    complete.insert(complete.end(), { 0x80, 1, 2, 3, 4, 5 });
    check(
        out.size() == 1 && out[0].bytes == complete && duplicates.stats().reordered == 3,
        "repeated adjacent FU fragments neither duplicate bytes nor publish the completed NAL twice");

    HevcRtpDepacketizer reordered;
    out.clear();
    for (const auto& p : { start, finish, middle }) {
        reordered.push_nals(p, out, err);
    }
    check(out.empty() && !reordered.mid_fragment(), "out-of-order FU parts do not reconstruct a truncated or shuffled NAL");

    // 同端口的其他 PT 不属于本次 FU，也不应破坏它；视频序号按自身连续。
    HevcRtpDepacketizer interleaved;
    out.clear();
    interleaved.push_nals(start, out, err);
    interleaved.push_nals(packet(900, 0, false, single(1, { 0x80 }), 101), out, err);
    interleaved.push_nals(middle, out, err);
    interleaved.push_nals(finish, out, err);
    check(
        out.size() == 1 && out[0].bytes == complete && interleaved.stats().other_payload == 1 && interleaved.stats().seq_gaps == 0,
        "another payload type is excluded from FU data and video sequence tracking");

    HevcRtpDepacketizer unidentified;
    out.clear();
    unidentified.push_nals(start, out, err);
    check(
        !unidentified.push_nals(std::vector<uint8_t> { 1, 2, 3 }, out, err) && unidentified.mid_fragment(),
        "an unidentifiable datagram does not discard a valid FU from another source");
    unidentified.push_nals(middle, out, err);
    unidentified.push_nals(finish, out, err);
    check(
        out.size() == 1 && out[0].bytes == complete && unidentified.stats().malformed == 1 && unidentified.stats().dropped_fragments == 0,
        "valid contiguous fragments still complete after unrelated malformed data");

    // RC=0 的合法 RTCP RR 只有八字节，不能被十二字节 RTP 头解析器识别。
    // 调用方可能把尚未分类的数据报送入低层入口；RR 不占用视频序号。
    HevcRtpDepacketizer short_rr;
    out.clear();
    short_rr.push_nals(start, out, err);
    const std::vector<uint8_t> rr { 0x80, 201, 0, 1, 0x12, 0x34, 0x56, 0x78 };
    check(
        !short_rr.push_nals(rr, out, err) && short_rr.mid_fragment() && out.empty(),
        "eight-byte RTCP RR does not discard an in-progress video FU");
    short_rr.push_nals(packet(11, 500, true, fragment(20, { 2, 3, 4, 5 }, false, true, 37, 4)), out, err);
    check(
        out.size() == 1 && out[0].bytes == complete && out[0].timestamp == 500 && out[0].ssrc == 0xdeadbeefU && out[0].first_sequence == 10
            && out[0].last_sequence == 11 && out[0].ends_access_unit && short_rr.stats().packets == 2 && short_rr.stats().seq_gaps == 0
            && short_rr.stats().dropped_fragments == 0,
        "contiguous FU end after short RR retains the original metadata and complete bytes");

    for (const auto& replacement : { single(1, { 0x80, 0x42 }), aggregation({ single(32, { 0x42 }) }) }) {
        HevcRtpDepacketizer d;
        out.clear();
        d.push_nals(start, out, err);
        d.push_nals(packet(11, 600, true, replacement), out, err);
        d.push_nals(finish, out, err);
        check(
            out.size() == 1 && out[0].timestamp == 600 && !d.mid_fragment(),
            "a new complete payload discards the preceding incomplete FU");
    }

    HevcRtpDepacketizer reset;
    out.clear();
    reset.push_nals(start, out, err);
    reset.reset();
    check(
        !reset.mid_fragment() && reset.fragment_timestamp() == 0 && reset.last_sequence() == 10 && reset.stats().dropped_fragments == 1,
        "explicit fragment reset clears bytes and metadata but preserves sequence accounting");
    reset.push_nals(finish, out, err);
    check(out.empty(), "a tail after reset cannot complete a NAL from the previous assembly");
}

std::vector<uint8_t> padded(std::vector<uint8_t> p, uint8_t count)
{
    p[0] |= 0x20;
    p.insert(p.end(), count, 0xa5);
    p.back() = count;
    return p;
}

void test_rtp_padding()
{
    std::printf("\n== RTP payload and padding boundaries ==\n");
    const auto original = single(1, { 0x80, 0x55, 3 });
    const auto valid = padded(packet(7, 800, true, original), 3);
    maa::ios::rt::PacketInfo info;
    check(
        maa::ios::rt::parse_rtp_header(valid, info) && info.payload_offset == 20 && info.payload_size == original.size(),
        "RTP accepts three padding bytes and reports only the actual payload size");
    HevcRtpDepacketizer typed;
    std::vector<maa::ios::rt::ReceivedNal> nals;
    std::string err;
    typed.push_nals(valid, nals, err);
    check(nals.size() == 1 && nals[0].bytes == original, "padding is absent from the complete NAL");

    HevcRtpDepacketizer fu;
    nals.clear();
    fu.push_nals(padded(packet(8, 900, false, fragment(20, { 0x80, 1 }, true, false)), 1), nals, err);
    fu.push_nals(padded(packet(9, 900, true, fragment(20, { 2, 3 }, false, true)), 255), nals, err);
    check(
        nals.size() == 1 && nals[0].bytes == single(20, { 0x80, 1, 2, 3 }) && nals[0].first_sequence == 8 && nals[0].last_sequence == 9,
        "FU assembly excludes one-byte and maximum-count RTP padding");
    nals.clear();
    fu.push_nals(padded(packet(10, 1000, true, aggregation({ original, original })), 7), nals, err);
    check(
        nals.size() == 2 && nals[0].bytes == original && nals[1].bytes == original && !nals[0].ends_access_unit && nals[1].ends_access_unit,
        "AP validation stops at payload_size rather than treating padding as another length");
    auto audio = padded(packet(11, 1100, false, { 0x41, 0x42, 0x43 }, 101), 5);
    check(
        maa::ios::rt::parse_rtp_header(audio, info) && info.payload_type == 101 && info.payload_size == 3,
        "valid padded audio RTP remains accepted by the shared header parser");

    std::vector<std::vector<uint8_t>> malformed;
    auto zero = valid;
    zero.back() = 0;
    malformed.push_back(zero);
    auto crosses_header = valid;
    crosses_header.back() = static_cast<uint8_t>(crosses_header.size());
    malformed.push_back(crosses_header);
    auto no_payload = valid;
    no_payload.back() = static_cast<uint8_t>(valid.size() - 20);
    malformed.push_back(no_payload);
    auto one_byte = valid;
    one_byte.back() = static_cast<uint8_t>(valid.size() - 21);
    malformed.push_back(one_byte);
    for (const auto& bad : malformed) {
        const maa::ios::rt::PacketInfo sentinel { true, 127, 60000, 1, 2, 3, 4 };
        info = sentinel;
        HevcRtpDepacketizer d;
        const maa::ios::rt::ReceivedNal old = { original, 1, 2, 3, 3, true };
        nals = { old };
        check(
            !maa::ios::rt::parse_rtp_header(bad, info) && info.marker == sentinel.marker && info.payload_type == sentinel.payload_type
                && info.sequence == sentinel.sequence && info.timestamp == sentinel.timestamp && info.ssrc == sentinel.ssrc
                && info.payload_offset == sentinel.payload_offset && info.payload_size == sentinel.payload_size,
            "invalid RTP padding leaves the caller's parsed header unchanged");
        check(
            !d.push_nals(bad, nals, err) && nals.size() == 1 && nals[0] == old && d.stats().malformed == 1,
            "invalid RTP padding cannot publish a NAL or replace existing output");
    }
}

void test_consecutive_fragments()
{
    std::printf("\n== Consecutive complete-NAL assemblies ==\n");
    HevcRtpDepacketizer d;
    std::vector<ReceivedNal> nals;
    std::string err;
    d.push_nals(packet(1, 400, false, fragment(20, { 0x80, 1 }, true, false)), nals, err);
    check(nals.empty(), "FU start does not publish a partial NAL");
    d.push_nals(packet(2, 400, true, fragment(20, { 2 }, false, true)), nals, err);
    check(
        nals.size() == 1 && nals[0] == ReceivedNal { single(20, { 0x80, 1, 2 }), 400, 0xdeadbeefU, 1, 2, true },
        "first FU publishes complete bytes and metadata");
    nals.clear();
    d.push_nals(packet(3, 800, false, fragment(1, { 0x80, 3 }, true, false)), nals, err);
    check(nals.empty(), "next FU start does not publish data from the previous assembly");
    d.push_nals(packet(4, 800, true, fragment(1, { 4 }, false, true)), nals, err);
    check(
        nals.size() == 1 && nals[0] == ReceivedNal { single(1, { 0x80, 3, 4 }), 800, 0xdeadbeefU, 3, 4, true } && d.stats().nals == 2,
        "next FU publishes only its own bytes and counts completed NALs once");
}

void test_invalid_nal_headers()
{
    std::printf("\n== Invalid NAL headers ==\n");
    std::vector<std::vector<uint8_t>> invalid;
    auto forbidden = single(1, { 0x80 });
    forbidden[0] |= 0x80;
    invalid.push_back(forbidden);
    auto no_temporal_id = single(1, { 0x80 });
    no_temporal_id[1] = 0;
    invalid.push_back(no_temporal_id);
    invalid.push_back(single(50, { 0x80 }));
    invalid.push_back(single(63, { 0x80 }));
    invalid.push_back(fragment(20, { 0x80 }, true, true));
    invalid.push_back(fragment(20, { }, true, false));
    invalid.push_back(nal_header(49));
    for (const auto& payload : invalid) {
        HevcRtpDepacketizer d;
        std::vector<maa::ios::rt::ReceivedNal> nals;
        std::string err;
        check(
            d.push_nals(packet(1, 400, true, payload), nals, err) && nals.empty() && !d.mid_fragment() && d.stats().malformed == 1,
            "invalid or unsupported HEVC payload does not escape as a complete NAL");
    }
}

void test_late_payload_isolation()
{
    std::printf("\n== Late RTP payload isolation ==\n");
    const auto idr = single(20, { 0x80, 1 });
    const auto next = single(1, { 0x80, 2 });
    for (const bool aggregate : { false, true }) {
        const auto old_payload = aggregate ? aggregation({ idr, single(40, { 3 }) }) : idr;
        HevcRtpDepacketizer typed;
        std::vector<maa::ios::rt::ReceivedNal> nals;
        std::string err;
        const auto old = packet(100, 400, true, old_payload);
        const auto later = packet(102, 1200, true, next);
        typed.push_nals(old, nals, err);
        typed.push_nals(later, nals, err);
        const auto before_nals = nals;
        // seq=101 从未交付，迟到时补齐接收缺口；seq=100 是重复旧 IDR。
        for (const auto& old_packet : { packet(101, 800, true, old_payload), old }) {
            check(typed.push_nals(old_packet, nals, err), "late/duplicate valid RTP is accepted for reception accounting");
        }
        check(
            nals == before_nals && typed.stats().seq_gaps == 1 && typed.stats().seq_lost == 0 && typed.stats().reordered == 2
                && typed.last_sequence() == 102,
            "late or repeated complete IDR cannot publish again through the complete-NAL output");
    }

    auto bad_old = idr;
    bad_old[0] |= 0x80;
    const std::vector<std::vector<uint8_t>> old_payloads { idr,
                                                           aggregation({ idr, single(40, { 3 }) }),
                                                           fragment(20, { 0x80, 9 }, true, false),
                                                           bad_old };
    for (const auto& old_payload : old_payloads) {
        HevcRtpDepacketizer typed;
        std::string err;
        std::vector<maa::ios::rt::ReceivedNal> nals;
        const auto initial = packet(100, 400, true, idr);
        const auto start = packet(101, 800, false, fragment(20, { 0x80, 4 }, true, false, 37, 4));
        const auto old = packet(100, 400, true, old_payload);
        // 相同最高序号但时间/标记/头部不同的副本也不能覆盖原始分片。
        const auto duplicate = packet(101, 999, true, fragment(21, { 0x80, 9 }, true, false));
        const auto finish = packet(102, 800, true, fragment(20, { 5 }, false, true, 37, 4));
        typed.push_nals(initial, nals, err);
        nals.clear();
        for (const auto& p : { start, old, duplicate }) {
            typed.push_nals(p, nals, err);
        }
        check(
            nals.empty() && typed.mid_fragment() && typed.fragment_timestamp() == 800 && typed.last_sequence() == 101
                && typed.stats().dropped_fragments == 0 && typed.stats().malformed == 0,
            "late single/AP/FU/bad payload and changed duplicate leave current FU untouched");
        typed.push_nals(finish, nals, err);
        auto complete = nal_header(20, 37, 4);
        complete.insert(complete.end(), { 0x80, 4, 5 });
        check(
            nals.size() == 1 && nals[0].bytes == complete && nals[0].timestamp == 800 && nals[0].ssrc == 0xdeadbeefU
                && nals[0].first_sequence == 101 && nals[0].last_sequence == 102 && nals[0].ends_access_unit && typed.stats().nals == 2
                && typed.stats().reordered == 2 && typed.stats().seq_gaps == 0 && typed.stats().seq_lost == 0
                && typed.stats().dropped_fragments == 0,
            "continuous FU end after old packets publishes original bytes and metadata once");
    }

    HevcRtpDepacketizer sampling_order;
    std::vector<maa::ios::rt::ReceivedNal> nals;
    std::string err;
    sampling_order.push_nals(packet(65535, 1200, true, idr), nals, err);
    sampling_order.push_nals(packet(0, 800, true, next), nals, err);
    check(
        nals.size() == 2 && nals[0].timestamp == 1200 && nals[1].timestamp == 800 && nals[1].first_sequence == 0
            && sampling_order.stats().reordered == 0 && sampling_order.stats().seq_gaps == 0,
        "forward RTP sequence across wrap retains backward sampling timestamps");
}

/// RTCP 那几种包的字节形状。
///
/// 为什么要钉：产品的会话续命现在依赖每秒发一个 RR（设备的 `RTCPTimeoutInterval` 是
/// "距离上次收到我们 RTCP 多久"的空闲计时器），而这个文件所在的那条链上，"形状写错"
/// 有过两次前科：一次把公共头的 16 位长度字段用 `put32` 写成 32 位（整包从第 3 字节起
/// 错位，于是"设备不理这种包"的结论建立在畸形包上），一次是数据报拼装把载荷留在缓冲区
/// 尾巴上（长度字段与校验和的范围都不对，设备内核静默丢弃）。两种在发送侧都不报错，
/// 只有对面能看出来——所以对面不在场时，必须靠这几条测。
void test_rtcp_shapes()
{
    std::printf("\n== RTCP 包形状 ==\n");
    const uint32_t our = 0x11223344u;   // 设备分配给我们这一端的 RemoteSSRC
    const uint32_t media = 0x55667788u; // 设备自己那条流的 LocalSSRC
    const uint32_t ext_high = 0x0001c8e0u;

    const auto rr = maa::ios::rt::build_rr(our, media, ext_high);
    check(rr.size() == 32, "RR 一共 32 字节: " + std::to_string(rr.size()));
    check(rr[0] == 0x81, "RR 首字节 0x81（V=2, RC=1）");
    check(rr[1] == 201, "RR 的 PT=201");
    check((rr[2] << 8 | rr[3]) == 7, "RR 长度字段=7（16 位，头之后 7 个字）");
    check(rr[4] == 0x11 && rr[7] == 0x44, "发送者 SSRC 在偏移 4，填的是设备分配的 RemoteSSRC");
    check(rr[8] == 0x55 && rr[11] == 0x88, "报告块指认的 SSRC 在偏移 8，填设备的 LocalSSRC");
    check(rr[16] == 0x00 && rr[17] == 0x01 && rr[18] == 0xc8 && rr[19] == 0xe0, "扩展最高序号在偏移 16");
    // 头 4 + 发送者 SSRC 4 + 报告块 24 = 32，与长度字段自洽：7 个字 + 首字 = 8 字。
    check(rr.size() == std::size_t(4 + 7 * 4), "长度字段 7 与真实字节数自洽");

    // SR 的 RC=0 形态不匹配设备心跳分类。
    std::vector<uint8_t> sr(28, 0);
    sr[0] = 0x80;
    sr[1] = 200;
    sr[3] = 6;
    std::vector<uint8_t> dev_sr(28, 0);
    dev_sr[0] = 0x81;
    dev_sr[1] = 0xc8; // PT=200
    check(maa::ios::rt::is_rtcp_sr(dev_sr), "0x81 0xc8 且 >=28 字节的认成 SR（设备那条心跳）");
    check(!maa::ios::rt::is_rtcp_sr(sr), "RC=0 的 SR（0x80 0xc8）不认成设备心跳");
    const auto pkt = packet(1, 0, true, single(1, { 0x01, 0x02 }));
    check(!maa::ios::rt::is_rtcp_sr(pkt), "RTP 视频包不认成 SR");
    check(!maa::ios::rt::is_rtcp_sr(std::span<const uint8_t>(rr)), "RR(PT=201) 不认成 SR");
    const std::vector<uint8_t> tiny = { 0x81, 0xc8 };
    check(!maa::ios::rt::is_rtcp_sr(tiny), "短于 28 字节的 0x81 0xc8 不认成 SR（不越界读）");

    // 校验 PLI 的公共头、包长和 SSRC 字节序。
    const auto pli = maa::ios::rt::build_pli(our, media);
    check(pli.size() == 12, "PLI 是 12 字节: " + std::to_string(pli.size()));
    check(pli[0] == 0x81 && pli[1] == 206, "PLI 首两字节 0x81 PT=206（PSFB）/FMT=1");
    check((pli[2] << 8 | pli[3]) == 2, "PLI 长度字段=2");
    check(pli.size() == std::size_t(4 + 2 * 4), "PLI 长度字段与真实字节数自洽");
    check(pli[4] == 0x11 && pli[11] == 0x88, "PLI 后面是发送者 SSRC + 媒体 SSRC");
}

void test_sender_reports()
{
    std::printf("\n== RTCP 发送者时钟解析 ==\n");
    const auto write32 = [](std::vector<uint8_t>& bytes, std::size_t at, uint32_t value) {
        for (int i = 0; i < 4; ++i) {
            bytes[at + i] = uint8_t(value >> (24 - i * 8));
        }
    };
    const std::vector<uint8_t> sr = {
        0x80, 0xc8, 0x00, 0x06, 0xb6, 0xd5, 0xb7, 0x56, 0xee, 0x5f, 0xe7, 0xad, 0xce, 0x84,
        0x90, 0x00, 0x00, 0x00, 0x57, 0xfb, 0x00, 0x00, 0x01, 0x6c, 0x00, 0x06, 0x21, 0x09,
    };
    const maa::ios::rt::SenderReport expected { 0xb6d5b756, 0xee5fe7ad, 0xce849000, 0x57fb, 364, 401673 };
    std::vector<maa::ios::rt::SenderReport> reports;
    check(
        maa::ios::rt::parse_sender_reports(sr, reports) && reports == std::vector { expected },
        "RC=0 SR 保留完整 NTP/RTP、SSRC 和发送计数");

    // RC=1 需要一个标准的24字节接收报告块；这里按设备52字节SR的布局构造。
    auto counted = sr;
    counted[0] = 0x81;
    counted[3] = 12;
    counted.resize(52, 0);
    const std::vector<uint8_t> sdes = { 0x81, 0xca, 0, 2, 0xb6, 0xd5, 0xb7, 0x56, 1, 0, 0, 0 };
    auto compound = counted;
    compound.insert(compound.end(), sdes.begin(), sdes.end());
    check(
        maa::ios::rt::parse_sender_reports(compound, reports) && reports == std::vector { expected },
        "52字节SR加SDES按各自长度关联，不误读报告块");

    auto second = sr;
    write32(second, 4, 42);
    write32(second, 16, 0xfffffff0);
    compound.insert(compound.end(), second.begin(), second.end());
    check(
        maa::ios::rt::parse_sender_reports(compound, reports) && reports.size() == 2 && reports[0] == expected && reports[1].ssrc == 42
            && reports[1].rtp_timestamp == 0xfffffff0,
        "复合包内多个SR保留各自时钟和顺序");
    check(maa::ios::rt::parse_sender_reports(maa::ios::rt::build_rr(1, 2, 3), reports) && reports.empty(), "有效RR没有SR，清空旧锚点");
    auto zero_ntp = sr;
    write32(zero_ntp, 4, 9);
    std::fill(zero_ntp.begin() + 8, zero_ntp.end(), 0);
    check(
        maa::ios::rt::parse_sender_reports(zero_ntp, reports) && reports.size() == 1 && reports[0].ntp_seconds == 0
            && reports[0].ntp_fraction == 0,
        "全零NTP保持原值，解析层不伪造时间");

    auto padded = sr;
    padded[0] |= 0x20;
    padded[3] = 7;
    padded.insert(padded.end(), { 0, 0, 0, 4 });
    check(maa::ios::rt::parse_sender_reports(padded, reports) && reports == std::vector { expected }, "末包有效填充不改变SR字段");
    auto extended = sr;
    extended[3] = 7;
    extended.insert(extended.end(), { 1, 2, 3, 4 });
    check(maa::ios::rt::parse_sender_reports(extended, reports) && reports == std::vector { expected }, "允许SR末尾profile扩展");

    check(
        maa::ios::rt::validate_rtcp(sr) && maa::ios::rt::validate_rtcp(compound)
            && maa::ios::rt::validate_rtcp(maa::ios::rt::build_rr(1, 2, 3)) && maa::ios::rt::validate_rtcp(padded)
            && maa::ios::rt::validate_rtcp(extended),
        "validation-only path accepts the same SR, compound, RR, padding and profile extensions");
    const auto reject = [&](const std::vector<uint8_t>& bytes, const std::string& why) {
        reports = { expected };
        check(
            !maa::ios::rt::validate_rtcp(bytes) && !maa::ios::rt::parse_sender_reports(bytes, reports)
                && reports == std::vector { expected },
            why + "；失败保留原输出");
    };
    bool truncations_rejected = true;
    for (std::size_t size = 0; size < sr.size(); ++size) {
        reports = { expected };
        truncations_rejected &= !maa::ios::rt::validate_rtcp(std::span<const uint8_t>(sr).first(size))
                                && !maa::ios::rt::parse_sender_reports(std::span<const uint8_t>(sr).first(size), reports)
                                && reports == std::vector { expected };
    }
    check(truncations_rejected, "SR每个截断位置均拒绝，且不交付部分锚点");
    auto bad = sr;
    bad[0] = 0x40;
    reject(bad, "拒绝非v2头");
    reject(packet(1, 2, false, single(1, { 0x80 })), "拒绝RTP媒体包");
    bad = sr;
    bad[0] = 0x81;
    reject(bad, "拒绝RC声明多于实际报告块");
    bad = maa::ios::rt::build_rr(1, 2, 3);
    bad[0] = 0x82;
    reject(bad, "拒绝不完整RR报告块");
    bad = padded;
    bad.back() = 0;
    reject(bad, "拒绝零长度填充");
    bad.back() = 3;
    reject(bad, "拒绝非四字节对齐的填充，不将残留字节当profile扩展");
    bad.back() = 28;
    reject(bad, "拒绝吞掉SR固定字段的填充");
    bad.back() = 252;
    reject(bad, "拒绝越过包边界的填充");
    bad = padded;
    bad.insert(bad.end(), sdes.begin(), sdes.end());
    reject(bad, "拒绝非末包的填充");
    bad = sr;
    bad.insert(bad.end(), { 0x80, 202, 0, 2, 0 });
    reject(bad, "有效SR后接截断SDES时整体拒绝");
    bad = sr;
    bad.push_back(0);
    reject(bad, "拒绝复合包末尾不足一个头的字节");
}

void test_sequence_reordering()
{
    std::printf("\n== 乱序到达不该被记成丢包 ==\n");
    // 这条链路上乱序是实测常态（一秒 100 个包的音频流里每 2 秒就有几次）。旧写法把
    // 序号水位更新成"最后**到达**的那个"，于是 100、102、101、103 这样一个都没丢的
    // 序列会被记成两次缺口：101 迟到把水位从 102 拽回 101，103 于是"跳"了一格。
    // 而 seq_gaps 在视频那条腿上是发 PLI、甚至重起整条会话的理由。
    RtpSeq seq;
    std::size_t gaps = 0, late = 0;
    const uint16_t arrival[] = { 100, 102, 101, 103 };
    for (const uint16_t s : arrival) {
        switch (seq.observe(s)) {
        case RtpSeq::Verdict::kGap:
            ++gaps;
            break;
        case RtpSeq::Verdict::kLate:
            ++late;
            break;
        default:
            break;
        }
    }
    // 三个量要分开看，它们各自回答一个问题：
    //   gaps_detected()  往前跳过几个号（1 个：101）——只增
    //   lost()           现在确实没到的有几个（0 个：101 后来到了）
    //   extended_high()  RR 要报的含回绕水位；high() 保留原 16 位序号
    check(gaps == 1 && late == 1, "四个包一个没丢：只有一处缺口事件、一次迟到");
    check(seq.gaps_detected() == 1, "缺口按号数累计一次，不因为 103 又跳一格而记两笔");
    check(seq.lost() == 0, "补齐之后真正没到的包数是 0");
    check(seq.high() == 103, "水位是见过的最高号，不是最后到达的 101");

    // 真丢一个、而且再也没来：这个数必须待在 1，不能被"后来又到了几个包"冲掉。
    RtpSeq gone;
    for (const uint16_t s : std::array<uint16_t, 4> { 100, 101, 103, 104 }) {
        gone.observe(s);
    }
    check(gone.lost() == 1 && gone.gaps_detected() == 1, "真丢一个就是 1");
    // 已经补过的洞上再来一次同一个号（重复包），不能把 lost() 往下多冲一格——
    // 否则丢包数会变成"看网络心情"的数，也就失去意义了。
    RtpSeq refilled;
    for (const uint16_t s : std::array<uint16_t, 4> { 100, 102, 101, 101 }) {
        refilled.observe(s);
    }
    check(refilled.lost() == 0, "同一个迟到包重复到达只冲销一次");
    RtpSeq dupthenloss;
    for (const uint16_t s : std::array<uint16_t, 4> { 100, 101, 101, 103 }) {
        dupthenloss.observe(s);
    }
    check(dupthenloss.lost() == 1, "重复包不算补齐：水位之后的洞仍然欠着");

    // 迟得太久就不算补齐了。101 这个洞在 101+1024 之后被判死，此后再到的 101
    // 只会是别的东西（换了流、或表早就不认它）。
    RtpSeq aged;
    check(aged.observe(100) == RtpSeq::Verdict::kFirst, "建水位");
    check(aged.observe(102) == RtpSeq::Verdict::kGap, "欠一个 101");
    check(aged.observe(1200) == RtpSeq::Verdict::kGap, "水位走远");
    check(aged.observe(101) == RtpSeq::Verdict::kLate, "101 现在才到：判迟到了");
    check(aged.lost() == aged.gaps_detected(), "洞判死之后迟到的包不再冲销，欠账只增不减");

    // 回绕：水位跨 65535 那一圈之后，正常往前走的包不能被判成倒退。
    RtpSeq wrap;
    check(wrap.observe(65534) == RtpSeq::Verdict::kFirst, "第一个包只建水位");
    check(wrap.observe(65535) == RtpSeq::Verdict::kInOrder, "65535 接得上");
    check(wrap.observe(0) == RtpSeq::Verdict::kInOrder, "回绕到 0 仍然算接上");
    check(wrap.observe(2) == RtpSeq::Verdict::kGap, "回绕之后缺口仍然数得出来");
    check(wrap.lost() == 1, "回绕之后的缺口记成 1 个");
    check(wrap.observe(1) == RtpSeq::Verdict::kLate, "回绕之后的迟到不误判成大片缺口");
    check(wrap.lost() == 0, "回绕之后的迟到补齐同样要冲销掉");

    // 一次跳掉整个重排窗口：多半是换了流而不是真丢一千个包。账要如实记 big，
    // 但别为此留一张满表——判成丢失是对的，只是别把内存吃在那儿。
    RtpSeq jump;
    jump.observe(100);
    check(jump.observe(2100) == RtpSeq::Verdict::kGap, "跳 2000 号算缺口事件");
    check(jump.lost() == 1999 && jump.observe(150) == RtpSeq::Verdict::kLate, "大跳的账留着，迟到的旧号不再改它");

    // 重复包：既不推进水位，也不算缺口。
    RtpSeq dup;
    dup.observe(7);
    check(dup.observe(7) == RtpSeq::Verdict::kLate && dup.high() == 7, "重复包记迟到");
    dup.reset();
    check(dup.observe(5000) == RtpSeq::Verdict::kFirst, "换会话要重置：新流的第一包不该对上旧水位算成丢几千个");
    dup.observe(5000);
    dup.observe(5002);
    dup.reset();
    check(dup.lost() == 0, "reset 要把欠账一起清掉，否则新会话第一秒就背着旧账");

    // 端到端：喂真的 RTP 包，判拆包器的账与它回 RR 用的那个数。
    HevcRtpDepacketizer d;
    std::string err;
    std::vector<ReceivedNal> out;
    for (const uint16_t s : arrival) {
        d.push_nals(packet(s, 1000, false, single(1, { 0xA4 })), out, err);
    }
    check(d.stats().seq_gaps == 1 && d.stats().seq_lost == 0, "拆包器：一次缺口，补齐之后真丢 0 个");
    check(d.stats().reordered == 1, "迟到单独计一位 reordered");
    check(d.last_sequence() == 103 && d.extended_sequence() == 103, "未回绕时原始与扩展最高序号相同，迟到不让水位回退");
    d.push_nals(packet(105, 1000, false, single(1, { 0xA4 })), out, err);
    check(d.stats().seq_gaps == 2 && d.stats().seq_lost == 1, "接着真丢一个 104：seq_lost 从 0 变 1，说明它是当前欠账而不是历史累计");
}

void test_extended_sequence()
{
    std::printf("\n== RTCP 扩展最高序号 ==\n");
    RtpSeq seq;
    check(seq.high() == 0 && seq.extended_high() == 0, "尚未收到媒体时两个序号为零");
    check(seq.observe(65535) == RtpSeq::Verdict::kFirst && seq.extended_high() == 65535, "首包建立随机初始水位，不假定此前已经回绕");
    check(seq.observe(0) == RtpSeq::Verdict::kInOrder && seq.extended_high() == 65536, "跨过 65535 时扩展序号增加一个周期");
    check(
        seq.observe(1) == RtpSeq::Verdict::kInOrder && seq.high() == 1 && seq.extended_high() == 65537,
        "65535 到 0 到 1 得到扩展水位 65537，原 16 位接口仍返回 1");
    const uint16_t late_values[] = { 65535, 0, 1 };
    for (const uint16_t late : late_values) {
        check(
            seq.observe(late) == RtpSeq::Verdict::kLate && seq.extended_high() == 65537 && seq.high() == 1,
            "旧周期迟到、同周期迟到与重复均不推进或回退扩展水位");
    }
    check(seq.observe(32769) == RtpSeq::Verdict::kLate && seq.extended_high() == 65537, "原半周期边界仍拒绝有歧义的进展，不增加回绕次数");

    RtpSeq gap;
    gap.observe(65534);
    check(
        gap.observe(1) == RtpSeq::Verdict::kGap && gap.extended_high() == 65537 && gap.lost() == 2,
        "缺口跨回绕时仍正确记两个丢包并推进扩展水位");
    check(
        gap.observe(65535) == RtpSeq::Verdict::kLate && gap.lost() == 1 && gap.extended_high() == 65537,
        "旧周期的迟到包可补齐缺口，但不增加或减少回绕周期");
    check(
        gap.observe(0) == RtpSeq::Verdict::kLate && gap.lost() == 0 && gap.extended_high() == 65537,
        "新周期的迟到包补齐另一缺口，扩展水位保持不变");

    RtpSeq multiple;
    multiple.observe(65534);
    bool in_order = true;
    for (uint32_t i = 1; i <= 131077; ++i) {
        in_order = (multiple.observe(static_cast<uint16_t>(65534u + i)) == RtpSeq::Verdict::kInOrder) && in_order;
    }
    check(
        in_order && multiple.high() == 3 && multiple.extended_high() == 196611 && multiple.lost() == 0,
        "连续收包跨三个边界仍累计正确，不增加额外丢包");
    multiple.reset();
    check(multiple.extended_high() == 0 && multiple.high() == 0 && multiple.lost() == 0, "reset 同时清空周期、原始序号和既有丢包统计");
    check(multiple.observe(5000) == RtpSeq::Verdict::kFirst && multiple.extended_high() == 5000, "新会话从其首包重新计数，不继承旧周期");

    HevcRtpDepacketizer video;
    std::string error;
    std::vector<ReceivedNal> out;
    const uint16_t video_sequences[] = { 65535, 0, 1, 0, 65535, 1, 2 };
    for (const uint16_t arrival : video_sequences) {
        video.push_nals(packet(arrival, 1000, false, single(1, { 0xA4 })), out, error);
    }
    check(
        video.last_sequence() == 2 && video.extended_sequence() == 65538 && video.stats().seq_gaps == 0 && video.stats().seq_lost == 0
            && video.stats().reordered == 3,
        "真实 HEVC 拆包器暴露扩展水位，重复和迟到统计不变");
    const auto rr = maa::ios::rt::build_rr(0x11223344, 0x55667788, video.extended_sequence());
    check(
        rr.size() == 32 && rr[16] == 0 && rr[17] == 1 && rr[18] == 0 && rr[19] == 2,
        "真实视频 RR 的线上偏移 16 包含完整的周期和最高序号");
    video.reset();
    check(video.extended_sequence() == 65538, "只丢未完成分片不重置当前会话的 RR 序号周期");
    HevcRtpDepacketizer new_session;
    check(new_session.extended_sequence() == 0, "会话重建使用的新拆包器不继承旧 RR 周期");
}

} // namespace

int main()
{
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    test_aggregation();
    test_fragmentation();
    test_lost_fragments();
    test_single_and_offsets();
    test_payload_type_filter();
    test_feeds_annexb_parser();
    test_complete_nal_metadata();
    test_aggregation_atomicity();
    test_fragment_integrity();
    test_rtp_padding();
    test_consecutive_fragments();
    test_invalid_nal_headers();
    test_late_payload_isolation();
    test_rtcp_shapes();
    test_sender_reports();
    test_sequence_reordering();
    test_extended_sequence();
    std::printf("\n%s (失败 %d 项)\n", Failures == 0 ? "全部通过" : "存在失败", Failures);
    return Failures == 0 ? 0 : 1;
}
