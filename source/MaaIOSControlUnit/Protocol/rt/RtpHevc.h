#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include "rt/RtpSeq.h"

namespace maa::ios::rt
{

/// 经 RTP 拆包得到的一个完整原始 NAL，不含 Annex-B 起始码。
/// 时间戳保持线上 32 位值；时钟频率、回绕展开及会话 epoch 由调用方管理。
/// FU 使用起始包的 timestamp、SSRC 和首序号，尾序号与 marker 来自结束包。
/// AP 中各 NAL 共用包的时间戳和序号，marker 只属于最后一个 NAL。
struct ReceivedNal
{
    std::vector<uint8_t> bytes;
    uint32_t timestamp = 0;
    uint32_t ssrc = 0;
    uint16_t first_sequence = 0;
    uint16_t last_sequence = 0;
    /// RTP marker 声明该 NAL 结束一个 AU，不证明 AU 中其他 NAL 全部收到。
    bool ends_access_unit = false;
    bool operator==(const ReceivedNal&) const = default;
};

/// CoreDevice 视频流的 HEVC RTP 载荷拆包，按当前适配的无 DONL 形态处理。
///
/// 已观察的视频包带 X=1 的 RTP 扩展头：profile 0x9011，扩展数据长度为一个
/// 32 位字，连同扩展头自身共 8 字节。解析器按包内声明长度跳过扩展，不依赖此固定值。
/// 扩展之后的 HEVC 载荷按 RFC 7798 的类型标记分派：
///
/// ```text
///   2 字节 NAL 头，type = (b0 >> 1) & 0x3F
///     48  聚合包：后面是 (2 字节大端长度 + NAL)*
///     49  分片：再跟 1 字节 FU 头（S=0x80 E=0x40 TU=0x3F），然后是分片数据
///     其它 单一 NAL，剩下整个载荷就是它
/// ```
///
/// 当前设备流不含 DONL，FU 头后直接连接分片数据；该约束不能推广到所有 HEVC RTP 流。
class HevcRtpDepacketizer
{
public:
    /// 仅处理指定的视频 PT，其他 PT 不进入 HEVC 载荷解析及视频序号统计。
    /// 当前设备的裸 RTCP 与 RTP 共用 UDP 端口，SR 的开头为 81 c8；若按 RTP
    /// 的 7 位 PT 解读，0xc8 会得到 72。上层需先区分 RTCP，避免非视频数据进入参考链。
    explicit HevcRtpDepacketizer(uint8_t video_payload_type = 100)
        : video_pt_(video_payload_type)
    {
    }

    struct Stats
    {
        uint64_t packets = 0;
        uint64_t nals = 0;
        /// 未完成 NAL 被作废，以及没有起始状态的 FU 被丢弃的次数。
        /// 每个孤立 FU 分别计一次，不能据此推断不同 NAL 的数量。
        uint64_t dropped_fragments = 0;
        uint64_t malformed = 0;
        /// seq_gaps 统计向前跳过序号的事件数，一次事件可以涉及多个缺失包。
        /// seq_lost 是 RtpSeq 当前未补齐的缺口数，窗口内迟到补齐会使其减少。
        /// reordered 包含迟到包和重复包；这些载荷不会交付，补齐 seq_lost
        /// 仅证明该序号到达，不代表媒体已按正确顺序组装或可用于解码。
        /// 分片缺失可能使图像不完整或破坏参考链，上层据此判断是否需要请求刷新或重建会话。
        uint64_t seq_gaps = 0;
        uint64_t seq_lost = 0;
        uint64_t reordered = 0;
        /// 非视频 PT（RTCP 等）被跳过的包数。
        uint64_t other_payload = 0;
    };

    /// 将本包完成的 NAL 追加到 out，不替换调用方已有内容。
    /// false 仅表示 RTP 头（含 padding）无效；HEVC 载荷异常返回 true 并计入
    /// malformed 或 dropped_fragments，不产出伪完整 NAL。AP 先验证全包再交付。
    /// 接受仅含一个 NAL 的 AP；RFC 7798 标准形态至少含两个 NAL。
    /// FU 不缓存乱序分片：缺口或身份不符会作废当前 NAL，等待下一个起始分片。
    /// 所有迟到/重复序号只更新接收统计，不交付载荷，也不改变当前 FU。
    /// 采样时间回退本身不被丢弃：序号向前的合法包仍可按原始时间交付。
    /// 只在组装 FU 时核对 SSRC，不代替调用方的源选择与会话隔离。
    bool push_nals(std::span<const uint8_t> datagram, std::vector<ReceivedNal>& out, std::string& err);

    /// 丢弃未完成的分片，用于切换显示、停止流或放弃当前图像。
    /// 不清空序号跟踪和累计统计。
    void reset();

    [[nodiscard]] const Stats& stats() const { return stats_; }

    /// 按模 2^16 顺序跟踪的视频包最高序号，尚未收到视频包时为 0。
    /// 迟到包不使其回退；此值为 16 位序号，不包含回绕次数。
    [[nodiscard]] uint16_t last_sequence() const { return seq_.high(); }

    /// 给 RTCP RR 的扩展最高序号，包含本会话的序号回绕次数。
    /// reset() 只丢未完成 FU，因此保留此值；新会话应使用新的拆包器。
    [[nodiscard]] uint32_t extended_sequence() const { return seq_.extended_high(); }

    /// 是否有还没收完的分片。
    [[nodiscard]] bool mid_fragment() const { return !partial_.bytes.empty(); }

    [[nodiscard]] uint32_t fragment_timestamp() const { return partial_.timestamp; }

private:
    uint8_t video_pt_;
    /// 统一跟踪序号进展、缺口和迟到补齐，统计规则见 RtpSeq。
    RtpSeq seq_;
    ReceivedNal partial_;
    Stats stats_;
};

/// RTP 头的解析结果，测试与上层统计都要用。
struct PacketInfo
{
    bool marker = false;
    uint8_t payload_type = 0;
    uint16_t sequence = 0;
    uint32_t timestamp = 0;
    uint32_t ssrc = 0;
    std::size_t payload_offset = 0; ///< 跳掉 RTP 固定头、CSRC 与扩展头之后的起点
    std::size_t payload_size = 0;   ///< 载荷字节数，不含 RTP padding
};

/// 校验 RTP v2、固定头、CSRC/扩展声明长度、padding，以及至少 2 字节实际载荷。
/// padding 的末字节计入其自身；RTP padding 不要求为四的倍数。
/// 仅成功时替换 out，失败保持原值。调用方使用 offset/size 取得不含填充的载荷。
[[nodiscard]] bool parse_rtp_header(std::span<const uint8_t> datagram, PacketInfo& out);

} // namespace maa::ios::rt
