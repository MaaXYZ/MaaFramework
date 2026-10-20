#include "media/MediaOffer.h"

#include "plist/Plist.h"
#include "util/Deflate.h"

namespace maa::ios::media
{
namespace
{

// 当前 offer 仅使用 protobuf 的 varint 和长度定界线型，不编码浮点或定宽字段。

void put_varint(std::vector<uint8_t>& out, uint64_t v)
{
    do {
        const auto byte = static_cast<uint8_t>(v & 0x7F);
        v >>= 7;
        out.push_back(v ? byte | 0x80 : byte);
    } while (v);
}

void put_field_varint(std::vector<uint8_t>& out, uint32_t field, uint64_t v)
{
    put_varint(out, field << 3 | 0);
    put_varint(out, v);
}

void put_field_bytes(std::vector<uint8_t>& out, uint32_t field, const std::vector<uint8_t>& b)
{
    put_varint(out, field << 3 | 2);
    put_varint(out, b.size());
    out.insert(out.end(), b.begin(), b.end());
}

void put_field_string(std::vector<uint8_t>& out, uint32_t field, std::string_view s)
{
    put_varint(out, field << 3 | 2);
    put_varint(out, s.size());
    out.insert(out.end(), s.begin(), s.end());
}

std::vector<uint8_t> msg(std::initializer_list<std::vector<uint8_t>> parts)
{
    std::vector<uint8_t> out;
    for (const auto& p : parts) {
        out.insert(out.end(), p.begin(), p.end());
    }
    return out;
}

std::vector<uint8_t> field_varint(uint32_t field, uint64_t v)
{
    std::vector<uint8_t> out;
    put_field_varint(out, field, v);
    return out;
}

// 各层字段号和默认值来自已成功建立的媒体会话样本；未确认的含义在对应字段处说明。

/// 分辨率条目的 f1..f4 为 {1, pair_index, 50115, 0}，pair_index 与 50115 的含义未确认。
/// 不将 pair_index 作为分辨率档位。
std::vector<uint8_t> res_entry(uint64_t pair_index)
{
    return msg({ field_varint(1, 1), field_varint(2, pair_index), field_varint(3, 50115), field_varint(4, 0) });
}

/// 编码器能力条目：f1 为 payload type，f2 为重复的分辨率条目，f3 为能力串，
/// f4 保留各编码器样本中的参数值。当前 offer 使用 PT 123 对应 HEVC、PT 100 对应 AVC；
/// 已测设备选择了 PT 100，因此对该次协商的 AVC 能力对照应修改对应条目。
/// HEVC 的四条与 AVC 的两条分辨率记录均来自样本；长度定界字段由实际内容计算。
/// 目前保留这一结构，不据样本长度推断设备要求所有 offer 逐字节一致。
std::vector<uint8_t> codec_bank(uint64_t payload_type, std::string_view features, uint64_t f4, uint64_t res_entries)
{
    std::vector<uint8_t> out = field_varint(1, payload_type);
    for (uint64_t i = 0; i < res_entries; ++i) {
        put_field_bytes(out, 2, res_entry(i % 2 + 1));
    }
    put_field_string(out, 3, features);
    auto tail = field_varint(4, f4);
    out.insert(out.end(), tail.begin(), tail.end());
    return out;
}

/// VideoSettings：客户端 SSRC、反馈与长期参考图选项、编码器能力和观测到的默认参数。
std::vector<uint8_t> session_blob(const Offer& offer)
{
    std::vector<uint8_t> out;
    put_field_varint(out, 1, offer.session_id);
    // `VCMediaNegotiationBlobVideoSettings` 的字段名：f1 SSRC(=session_id)、f2 allowRTCPFB、f3 videoPayloadCollections
    // （编码器能力条目）、f4 customVideoWidth、f5 customVideoHeight、f6 tilesPerFrame、
    // f7 ltrpEnabled、f8 pixelFormats、f9 hdrModesSupported、f10 fecEnabled、f11 rtxEnabled、
    // f12 blackFrameOnClearScreenEnabled、f13 foveationSupported、f14 enableInterleavedEncoding。
    // 按恢复的名称，f8=63 可能是像素格式位掩码（低六位为 1，具体格式未确认），
    // f10=1 对应 fecEnabled，f12=1 对应 blackFrameOnClearScreenEnabled。字段名提供
    // 解释线索，不代表各项功能或所有取值都受支持。省略 f6/f9/f11/f13/f14。
    //
    // 按字段号递增写入并保留默认结构，便于对照序列化样本。allowRTCPFB 与
    // ltrpEnabled 的对照范围见 MediaOffer.h；不将这种写入顺序或样本字节数视为
    // 设备通用约束，也不假定未知字段的省略与显式写零完全等价。
    put_field_varint(out, 2, offer.allow_rtcp_fb ? 1 : 0);
    put_field_bytes(out, 3, codec_bank(123, offer.hevc_features, 1, 4));
    put_field_bytes(out, 3, codec_bank(100, offer.avc_features, 14, 2));
    put_field_varint(out, 7, offer.ltrp_enabled ? 1 : 0);
    put_field_varint(out, 8, 63);
    put_field_varint(out, 10, 1);
    put_field_varint(out, 12, 1);
    return out;
}

/// 码率参数表，记录保存在重复的 f9 中。部分 f2 值与码率相关，f1/f3 的完整含义
/// 尚未确认，不能仅凭数值将整张表解释为编码器码率或 QP 的直接配置接口。
/// 超大 NAL 的处理由解码后端承担，不能依赖此表保证长度上限。
std::vector<uint8_t> rate_table()
{
    std::vector<uint8_t> out;
    auto entry = [&out](std::vector<std::pair<uint32_t, uint64_t>> fields) {
        std::vector<uint8_t> body;
        for (const auto& [field, value] : fields) {
            put_field_varint(body, field, value);
        }
        put_field_bytes(out, 9, body);
    };
    // 样本每条都包含 f1，包括值为 0 的记录。保留显式零值以维持已验证的编码形状，
    // 不在未知消息定义的情况下假定字段省略与显式写零完全等价。
    const std::vector<std::vector<std::pair<uint32_t, uint64_t>>> observed = {
        { { 1, 4074 }, { 2, 0 }, { 3, 16384 } },        { { 1, 0 }, { 2, 75'000'000 }, { 3, 524288 } },
        { { 1, 0 }, { 2, 40'000'000 }, { 3, 12288 } },  { { 1, 16 }, { 2, 4100 } },
        { { 1, 0 }, { 2, 20'000'000 }, { 3, 98304 } },  { { 1, 4 }, { 2, 6500 } },
        { { 1, 0 }, { 2, 6'000'000 }, { 3, 131072 } },  { { 1, 0 }, { 2, 100'000'000 }, { 3, 1'048'576 } },
        { { 1, 0 }, { 2, 60'000'000 }, { 3, 262144 } }, { { 1, 1 }, { 2, 299 } },
    };
    for (const auto& e : observed) {
        entry(e);
    }
    return out;
}

std::vector<uint8_t> media_blob(const Offer& offer)
{
    std::vector<uint8_t> out;
    put_field_varint(out, 1, 1);
    put_field_varint(out, 2, 1);
    put_field_bytes(out, 5, session_blob(offer));
    put_field_string(out, 6, "Viceroy 1.7.0");
    put_field_varint(out, 8, 0);
    auto rates = rate_table();
    out.insert(out.end(), rates.begin(), rates.end());
    // f13 的含义和设备校验方式尚未确认，保留可用会话中的固定值。
    put_field_varint(out, 13, 17'137'042'128'614'416'384ULL);
    put_field_varint(out, 14, 2);
    put_field_varint(out, 16, 0);
    put_field_varint(out, 18, 1);
    return out;
}

std::vector<uint8_t> remote_endpoint_info(const Offer& offer)
{
    std::vector<uint8_t> out;
    put_field_varint(out, 1, 0);
    put_field_varint(out, 2, 1);
    put_field_string(out, 3, offer.host_model);
    put_field_string(out, 4, offer.host_os_version);
    put_field_string(out, 5, offer.host_build);
    return out;
}

} // namespace

std::vector<uint8_t> build_negotiator_offer(const Offer& offer)
{
    auto d = plist::Value::Dict();
    d.set("avcMediaStreamNegotiatorMediaBlob", plist::Value::OfData(util::zlib_store(media_blob(offer))));
    d.set("avcMediaStreamNegotiatorMode", plist::Value::Int(5));
    d.set("avcMediaStreamOptionCallID", plist::Value::Str(offer.call_id));
    d.set("avcMediaStreamOptionRemoteEndpointInfo", plist::Value::OfData(remote_endpoint_info(offer)));
    const auto xml = plist::write(d);
    return { xml.begin(), xml.end() };
}

} // namespace maa::ios::media
