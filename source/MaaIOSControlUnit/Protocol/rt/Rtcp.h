#pragma once

#include <cstdint>
#include <span>
#include <vector>

namespace maa::ios::rt
{

/// SR 中的一组媒体时钟锚点。NTP 保留原始秒/分数值，不在解析时假定其为
/// 日历时间；发送端也可使用共同的相对时钟，或用全零表示没有时钟信息。
/// RTP 时间戳与该 NTP 时刻对应，不必等于相邻媒体包的时间戳。
struct SenderReport
{
    uint32_t ssrc = 0;
    uint32_t ntp_seconds = 0;
    uint32_t ntp_fraction = 0;
    uint32_t rtp_timestamp = 0;
    uint32_t packet_count = 0;
    uint32_t octet_count = 0;
    bool operator==(const SenderReport&) const = default;
};

/// 从单个或复合 RTCP 数据报提取全部 SR。校验公共头、声明长度、末包填充，
/// 以及 SR/RR 的固定字段与 RC 个 24 字节报告块；其他 PT 仅校验帧边界。
/// 允许独立 SR 及 profile 扩展，不要求完整会话的 SDES/CNAME 策略。
/// 成功替换 reports（无 SR 时为空）；失败保留原值，不交付有效前缀的锚点。
/// 本接口不替代现有设备心跳分类，也不证明音视频发送端的 NTP 时钟同步。
[[nodiscard]] bool parse_sender_reports(std::span<const uint8_t> datagram, std::vector<SenderReport>& reports);

/// RTCP 报告、源描述和反馈包的字节构造。
///
/// 当前设备媒体会话需要定期发送 RTCP，以维持 startmediastream 的 timeout
/// 所对应的空闲计时。已有同设备探针观察中，不发送 RTCP 时约 20 秒结束，
/// 每秒发送 RR 可持续超过 40 秒；该观察不保证所有设备和会话策略相同。
///
/// RFC 3550 §6.1 公共头为 4 字节：V/RC 1 字节、PT 1 字节、length 2 字节。
/// length 是整个 RTCP 包的 32 位字数减一，各构造函数的长度由离线用例检查。
/// 产品使用 RR 保活，并通过 PLI 请求关键帧。

/// RR（RFC 3550 §6.4.2）：V=2、RC=1、PT=201、length=7，共 32 字节，
/// 包含发送者 SSRC 和一个 24 字节接收报告块。
/// 当前设备协商中 sender_ssrc 对应 answer.RemoteSSRC，media_ssrc 对应
/// answer.LocalSSRC；Local/Remote 是设备视角，LocalSSRC 与设备发出的 RTP SSRC 对应。
/// ext_high 是报告块的 32 位扩展最高序号，包含序号回绕计数。
/// 当前构造器将丢包比例、累计丢包、抖动、LSR 和 DLSR 写为 0，未填入实际接收统计。
[[nodiscard]] std::vector<uint8_t> build_rr(uint32_t sender_ssrc, uint32_t media_ssrc, uint32_t ext_high);

/// PLI（RFC 4585 §6.3.1，PT=206/PSFB、FMT=1、length=2）：共 12 字节，
/// 仅含公共头、发送者 SSRC 和媒体 SSRC，没有 FCI。
/// 表示图像数据丢失、预测参考链可能受损；是否发送帧内刷新由发送端决定，不保证返回 IDR。
[[nodiscard]] std::vector<uint8_t> build_pli(uint32_t sender_ssrc, uint32_t media_ssrc);

/// 识别当前设备已观察到的 SR 心跳形态：至少 28 字节，开头为 81 c8（V=2、RC=1、PT=200）。
/// 此检查不解析长度字段和报告块，不是通用 RTCP SR 校验；RC=0 的 SR 不匹配此形态。
/// 当前设备的 SR 与视频共用 UDP 端口，画面静止时仍可观察到 SR；上层可分别记录
/// 媒体与心跳活动，结合超时判断会话状态，单个 SR 本身不能证明解码参考链健康。
[[nodiscard]] bool is_rtcp_sr(std::span<const uint8_t> datagram);

} // namespace maa::ios::rt
