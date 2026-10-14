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
/// 已测设备将 pair_index 从 0 改到 6 时，编码尺寸仍为 1136x2464；该结果不足以将它
/// 解释为分辨率档位。对照范围见 docs/coredevice.md §11，不能直接用于其他设备。
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
    // 参考实现根据 `VCMediaNegotiationBlobVideoSettings` 的 `__objc_methname` 恢复了
    // 以下字段名：f1 SSRC(=session_id)、f2 allowRTCPFB、f3 videoPayloadCollections
    // （编码器能力条目）、f4 customVideoWidth、f5 customVideoHeight、f6 tilesPerFrame、
    // f7 ltrpEnabled、f8 pixelFormats、f9 hdrModesSupported、f10 fecEnabled、f11 rtxEnabled、
    // f12 blackFrameOnClearScreenEnabled、f13 foveationSupported、f14 enableInterleavedEncoding。
    // 按恢复的名称，f8=63 可能是像素格式位掩码（低六位为 1，具体格式未确认），
    // f10=1 对应 fecEnabled，f12=1 对应 blackFrameOnClearScreenEnabled。字段名提供
    // 解释线索，不代表已经验证各项功能或所有取值；参考实现记录 f11=1 被拒为
    // Invalid Parameter。样本未包含 f6/f9/f13/f14，因此当前省略这些字段。
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
///
/// 参考实现曾将 f2、f3 分别或同时缩为 0.25；该次主屏 IDR 大小为
/// 49652 / 49789 / 49852 字节，基线为 49652 字节，没有观察到明显缩小。
/// 本项目的档位删除和替换对照见 MediaOffer.h 与 docs/coredevice.md §11：
/// 已测修改未提高 TXMaxBitrate，部分删除操作降低码率或帧率，因此产品保留原表。
/// 这些结果只限定于已测配置；超大 NAL 的处理由解码后端承担，不能依赖此表保证上限。
std::vector<uint8_t> rate_table(const Offer& offer)
{
    std::vector<uint8_t> out;
    auto entry = [&out](std::vector<std::pair<uint32_t, uint64_t>> fields) {
        std::vector<uint8_t> body;
        for (const auto [field, value] : fields) {
            put_field_varint(body, field, value);
        }
        put_field_bytes(out, 9, body);
    };
    // 样本每条都包含 f1，包括值为 0 的记录。保留显式零值以维持已验证的编码形状，
    // 不在未知消息定义的情况下假定字段省略与显式写零完全等价。
    auto f2_of = [](const std::vector<std::pair<uint32_t, uint64_t>>& e) {
        for (const auto [f, v] : e) {
            if (f == 2) {
                return v;
            }
        }
        return uint64_t { 0 };
    };
    const std::vector<std::vector<std::pair<uint32_t, uint64_t>>> observed = {
        { { 1, 4074 }, { 2, 0 }, { 3, 16384 } },        { { 1, 0 }, { 2, 75'000'000 }, { 3, 524288 } },
        { { 1, 0 }, { 2, 40'000'000 }, { 3, 12288 } },  { { 1, 16 }, { 2, 4100 } },
        { { 1, 0 }, { 2, 20'000'000 }, { 3, 98304 } },  { { 1, 4 }, { 2, 6500 } },
        { { 1, 0 }, { 2, 6'000'000 }, { 3, 131072 } },  { { 1, 0 }, { 2, 100'000'000 }, { 3, 1'048'576 } },
        { { 1, 0 }, { 2, 60'000'000 }, { 3, 262144 } }, { { 1, 1 }, { 2, 299 } },
    };
    for (auto e : observed) {
        if (offer.rate_variant == 1 && f2_of(e) == 6'000'000) {
            continue; // 对照变体 1：删除 f2=6000000 的记录。
        }
        if (offer.rate_variant == 2 && f2_of(e) == 6'000'000) {
            for (auto& [f, v] : e) {
                if (f == 2) {
                    v = 60'000'000;
                }
            }
        }
        if (offer.rate_variant == 3 && f2_of(e) >= 1000 && f2_of(e) < 20'000'000) {
            continue; // 对照变体 3：删除 1000 <= f2 < 20000000 的记录。
        }
        entry(e);
    }
    return out;
}

/// 音频设置放在媒体参数容器的 f3；视频设置使用 f5（VideoSettings）。
///
/// 结构来自 Xcode DeviceHub 的音频样本：negotiatorOffer 为 433 字节，解开后的
/// 媒体参数为 169 字节。f1 是客户端声明的 SSRC；已测 answer 的 RemoteSSRC 回显
/// 该值，客户端 RTCP 使用本次 answer 中的值作为发送者 SSRC。
/// f4=24191 的含义未确认；对应音频 streamConfig 观察到 AudioStreamMode=8、
/// RxPayloadType=101。f2/f3/f5/f6 保留样本中的零值，当前不赋予未验证的字段名称。
std::vector<uint8_t> audio_settings_blob(const Offer& offer)
{
    std::vector<uint8_t> out;
    put_field_varint(out, 1, offer.session_id);
    put_field_varint(out, 2, 0);
    put_field_varint(out, 3, 0);
    put_field_varint(out, 4, 24191);
    put_field_varint(out, 5, 0);
    put_field_varint(out, 6, 0);
    return out;
}

std::vector<uint8_t> media_blob(const Offer& offer)
{
    std::vector<uint8_t> out;
    put_field_varint(out, 1, 1);
    put_field_varint(out, 2, 1);
    if (offer.is_audio) {
        put_field_bytes(out, 3, audio_settings_blob(offer));
    }
    else {
        put_field_bytes(out, 5, session_blob(offer));
    }
    put_field_string(out, 6, "Viceroy 1.7.0");
    put_field_varint(out, 8, 0);
    auto rates = rate_table(offer);
    out.insert(out.end(), rates.begin(), rates.end());
    // f13 保留可用会话中的 64 位常量，参考实现也使用固定值。它曾被描述为时间戳，
    // 但含义和设备校验方式尚未确认；当前配置成功不代表任意值或过期值都可被接受。
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
    d.set("avcMediaStreamNegotiatorMode", plist::Value::Int(offer.is_audio ? (offer.audio_dup ? 6 : 10) : 5));
    d.set("avcMediaStreamOptionCallID", plist::Value::Str(offer.call_id));
    d.set("avcMediaStreamOptionRemoteEndpointInfo", plist::Value::OfData(remote_endpoint_info(offer)));
    const auto xml = plist::write(d);
    return { xml.begin(), xml.end() };
}

} // namespace maa::ios::media
