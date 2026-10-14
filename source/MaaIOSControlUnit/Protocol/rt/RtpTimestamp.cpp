#include "rt/RtpTimestamp.h"

#include <limits>

namespace maa::ios::rt
{

std::optional<int64_t> RtpTimestamp::nearest(uint32_t timestamp, int64_t reference) noexcept
{
    constexpr uint32_t half_cycle = uint32_t { 1 } << 31;
    constexpr int64_t cycle = int64_t { 1 } << 32;
    // 转成无符号低 32 位，再按模 2^32 相减。避免将超出 INT32_MAX 的值
    // 直接转成 int32_t，也避免在 64 位极值附近先计算可能溢出的候选周期。
    const uint32_t forward = timestamp - static_cast<uint32_t>(reference);
    if (forward == half_cycle) {
        return std::nullopt;
    }
    const int64_t delta = forward < half_cycle ? static_cast<int64_t>(forward) : static_cast<int64_t>(forward) - cycle;
    if ((delta > 0 && reference > std::numeric_limits<int64_t>::max() - delta)
        || (delta < 0 && reference < std::numeric_limits<int64_t>::min() - delta)) {
        return std::nullopt;
    }
    return reference + delta;
}

std::optional<int64_t> RtpTimestamp::observe(uint32_t timestamp) noexcept
{
    if (!high_) {
        high_ = static_cast<int64_t>(timestamp);
        return high_;
    }
    const auto value = nearest(timestamp, *high_);
    if (value && *value > *high_) {
        high_ = value;
    }
    return value;
}

} // namespace maa::ios::rt
