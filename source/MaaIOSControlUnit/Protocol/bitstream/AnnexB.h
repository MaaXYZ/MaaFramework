#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <utility>
#include <vector>

namespace maa::ios
{

/// 一个 NAL，包含两字节 HEVC NAL 头，字节保持与 Annex-B 起始码之后的载荷一致。
/// 保留 emulation prevention byte（EPB），便于将原始 NAL 传给解码器。
using Nal = std::vector<uint8_t>;

enum class NalType : uint8_t
{
    Vps = 32,
    Sps = 33,
    Pps = 34,
    SeiPrefix = 39,
};

/// Annex-B 字节或完整 NAL -> Access Unit 解析器。
///
/// 根据 slice segment header 的 first_slice_segment_in_pic_flag 判断新图像，
/// 同一图像中的后续 slice 继续合并；不能仅凭 NAL 类型划分多 slice 的 AU。
/// 头部信号不能证明传输没有丢片；调用方确认损失时必须 discard_pending。
/// 调用方为每个来源/会话 epoch 使用独立实例；这里不推导时钟频率、SSRC 或 DTS。
class AnnexBParser
{
public:
    struct AccessUnit
    {
        std::vector<Nal> nals;
        /// 沿用现有含 IRAP（类型 16..21）的标记，不保证每种 IRAP 都能独立起录。
        bool keyframe = false;
        /// 第一条 VCL 的采样 ticks，由调用方提前展开；前缀 NAL 不决定此值。
        std::optional<int64_t> sampling_timestamp;
    };

    using AuHandler = std::function<void(std::vector<Nal>&& au, bool keyframe)>;
    using TimedAuHandler = std::function<void(AccessUnit&& au)>;

    explicit AnnexBParser(AuHandler on_au)
        : on_au_([handler = std::move(on_au)](AccessUnit&& au) { handler(std::move(au.nals), au.keyframe); })
    {
    }

    explicit AnnexBParser(TimedAuHandler on_au)
        : on_au_(std::move(on_au))
    {
    }

    /// 两种输入方式不能在未 flush/discard_pending 的状态下混用。
    /// 返回 false 表示输入模式冲突（或非空输入的 data 为空），状态保持不变。
    /// Annex-B 本身没有采样 ticks，输出的 sampling_timestamp 为 nullopt。
    bool feed(const uint8_t* data, std::size_t len);

    /// 输入不含起始码的完整 NAL，保留原始 EPB。marker 在本条 NAL 后关闭 AU，
    /// 仅有参数/前缀的 AU 不交付。同一 AU 的各条 VCL 必须具有相同的 optional
    /// ticks（全 unknown 允许）；不同值或 known/unknown 混合会丢弃当前 AU。
    /// 返回 false 表示模式冲突、NAL 过短或残缺 slice/时间不一致。
    bool push_nal(Nal nal, std::optional<int64_t> sampling_timestamp = std::nullopt, bool marker = false);

    /// 调用方确认流结束时提交最后一条字节 NAL 和待提交 AU；随后可切换输入方式。
    /// 已确认丢片时应调用 discard_pending，不能用 flush 提交残缺图像。
    void flush();

    /// 丢弃未提交 AU 和尚未定界的字节，不触发回调；保留已缓存 VPS/SPS/PPS。
    void discard_pending();

    const Nal& vps() const { return vps_; }

    const Nal& sps() const { return sps_; }

    const Nal& pps() const { return pps_; }

    bool has_parameter_sets() const { return !vps_.empty() && !sps_.empty() && !pps_.empty(); }

private:
    void emit_range(std::size_t end);
    bool on_nal(Nal&& raw, std::optional<int64_t> sampling_timestamp);
    void close_au();
    void discard_au();

    TimedAuHandler on_au_;
    enum class InputMode
    {
        None,
        Bytes,
        Nals
    };
    InputMode input_mode_ = InputMode::None;

    std::vector<uint8_t> pending_; ///< 尚未定界的字节
    std::size_t nal_begin_ = 0;    ///< 当前 NAL 负载起点（起始码之后）
    std::size_t scan_from_ = 0;    ///< pending_ 中下一个待检查的起始码位置

    std::vector<Nal> cur_au_;
    bool au_open_ = false;
    /// cur_au_ 是否包含 VCL NAL。仅在包含图像数据时提交 AU，
    /// VPS/SPS/PPS 前缀继续保留，与随后到达的图像组成同一 AU。
    bool au_has_vcl_ = false;
    bool au_keyframe_ = false;
    std::optional<int64_t> au_sampling_timestamp_;

    Nal vps_, sps_, pps_;
};

/// 将完整的 00 00 03 xx（xx<=0x03）转换为 00 00 xx，保留截断或不符合该模式的字节。
/// 返回的 RBSP 仅用于读取语法元素，例如 SPS 的 conformance window 和 profile/tier。
/// 本项目的解码样本及 hvcC 参数集使用原始 NAL，保留 EPB；不要用 RBSP 替换这些输入。
std::vector<uint8_t> unescape_nal(const uint8_t* data, std::size_t len);

} // namespace maa::ios
