#pragma once

#include <cstdint>
#include <optional>

namespace maa::ios::rt
{

/// 将同一会话的 RTP 32 位时间戳展开为 64 位值，不改变其采样时间顺序。
///
/// RTP 的传输顺序可以不同于呈现顺序，迟到包也可能带着较早的时间戳。
/// 因此 observe() 可以返回小于之前输出的值，高水位只用于选择最近的回绕周期。
/// 本工具不保存频率、SSRC 或会话编号，不判断媒体是否重启；调用方确认进入新
/// epoch 时显式 reset()，不能仅因时间戳回退而清空状态。
///
/// 最近周期的选择要求实际时间戳离参考值不到 2^31 ticks。恰好半周期有两个
/// 同样近的候选，工具会拒绝；更长的失联或跳变也不能仅靠 32 位值可靠定位，
/// 调用方需要其他可辨识的锚点或明确的新 epoch。本工具不会推断这些条件。
/// 同一实例的 observe() 和 reset() 由调用方串行执行。
class RtpTimestamp
{
public:
    /// 首次观察返回原始无符号值，例如 0xffffffff 展开为 4294967295。
    /// 后续返回最邻近高水位周期的值；零附近的较早时间戳可能展开为负值。
    /// 半周期歧义或 64 位结果溢出时返回 nullopt，高水位保持不变。
    [[nodiscard]] std::optional<int64_t> observe(uint32_t timestamp) noexcept;

    /// 清空参考状态。下一次 observe() 按新会话的第一包处理。
    void reset() noexcept { high_.reset(); }

    /// 已观察到的最高展开值；未观察或 reset() 后为空。迟到及重复不降低此值。
    [[nodiscard]] std::optional<int64_t> high() const noexcept { return high_; }

    /// 围绕指定的 64 位参考选择最近周期，不修改实例状态。
    /// 可将同一源的 SR 时间戳放入已有 RTP 周期；它不验证 SR 的来源或时钟。
    /// 半周期歧义或结果超出 int64_t 范围时返回 nullopt。
    [[nodiscard]] static std::optional<int64_t> nearest(uint32_t timestamp, int64_t reference) noexcept;

private:
    std::optional<int64_t> high_;
};

} // namespace maa::ios::rt
