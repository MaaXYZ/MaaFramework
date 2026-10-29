#pragma once

#include <algorithm>
#include <chrono>
#include <thread>

#include "Task/Context.h"

MAA_TASK_NS_BEGIN

namespace interruptible_sleep
{

// 轮询粒度上限，兼顾停止响应速度与唤醒开销
inline constexpr std::chrono::milliseconds kPollInterval = std::chrono::milliseconds(50);

// post_stop 是立即停止语义：阻塞点必须尽快返回，不能一路睡满 timeout。
// context 为空时退化为普通 sleep。
inline void sleep_until(std::chrono::steady_clock::time_point deadline, Context* context)
{
    while (std::chrono::steady_clock::now() < deadline) {
        if (context && context->need_to_stop()) {
            return;
        }

        // 向上取整：剩余不足 1ms 时 duration_cast 会截断成 0，sleep_for(0) 会退化成忙等
        const auto remaining = std::chrono::ceil<std::chrono::milliseconds>(deadline - std::chrono::steady_clock::now());
        std::this_thread::sleep_for(std::min(kPollInterval, remaining));
    }
}

inline void sleep_for(std::chrono::milliseconds duration, Context* context)
{
    sleep_until(std::chrono::steady_clock::now() + duration, context);
}

} // namespace interruptible_sleep

MAA_TASK_NS_END
