#include "rt/RtpSeq.h"

#include <vector>

namespace maa::ios::rt
{

void RtpSeq::remember_holes(const uint16_t from, const uint32_t count)
{
    if (count >= kReorderWindow) {
        // 大跳跃仍计入 detected_，但不创建待补记录，限制缓存和后续查找开销。
        // 这是当前统计策略，不能单凭此跳跃判断是否更换了 SSRC 或会话。
        return;
    }
    if (pending_.capacity() < kReorderWindow) {
        pending_.reserve(kReorderWindow); // 预留一个窗口的容量，减少登记缺口时的分配
    }
    for (uint32_t i = 0; i < count; ++i) {
        pending_.push_back(static_cast<uint16_t>(from + i));
    }
}

void RtpSeq::prune()
{
    // uint16_t(high_ - s) 是模 2^16 的落后距离，达到 1024 后删除待补记录。
    // 单次向前推进最多 32767，推进后立即裁剪，不让旧记录保留到下一次序号回绕。
    std::erase_if(pending_, [this](const uint16_t s) { return static_cast<uint32_t>(static_cast<uint16_t>(high_ - s)) >= kReorderWindow; });
}

RtpSeq::Verdict RtpSeq::observe(const uint16_t seq)
{
    if (!have_) {
        have_ = true;
        high_ = seq;
        return Verdict::kFirst;
    }
    // 差值按模 2^16 解释：(0, 32768) 视为向前，0 视为重复，>=32768 视为迟到。
    // 该半序号空间规则覆盖 65535 到 0 的回绕；超大跳跃本身无法消除顺序歧义。
    const unsigned forward = static_cast<unsigned>(static_cast<uint16_t>(seq - high_));
    if (forward >= 32768) {
        // 只有仍在待补记录中的迟到序号才增加 filled_。
        // 已补齐或过期的序号再次到达不会重复减少 lost()。
        filled_ += std::erase(pending_, seq);
        return Verdict::kLate; // 迟到包不推进最高序号
    }
    if (forward == 0) {
        return Verdict::kLate; // 与最高序号相同，属于重复包
    }
    // 先登记跳过的缺口，再推进最高序号并裁剪过期待补记录。
    if (forward > 1) {
        remember_holes(static_cast<uint16_t>(high_ + 1), forward - 1);
        detected_ += forward - 1;
    }
    if (seq < high_) {
        cycles_ += uint32_t { 1 } << 16;
    }
    high_ = seq;
    prune();
    return forward > 1 ? Verdict::kGap : Verdict::kInOrder;
}

} // namespace maa::ios::rt
