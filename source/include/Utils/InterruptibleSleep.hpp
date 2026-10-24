#pragma once

#include <algorithm>
#include <chrono>
#include <functional>
#include <thread>

#include "Common/Conf.h"

MAA_TASK_NS_BEGIN

// 轮询粒度上限，兼顾停止响应速度与唤醒开销
inline constexpr std::chrono::milliseconds kInterruptibleSleepPollInterval = std::chrono::milliseconds(50);

// 可在阻塞中途被外部条件打断的 sleep。
// post_stop 是立即停止语义：阻塞点必须尽快返回，否则 sleep 会一路跑满 timeout。
inline void sleep_for_interruptible(std::chrono::milliseconds duration, const std::function<bool()>& interrupted)
{
    const auto deadline = std::chrono::steady_clock::now() + duration;

    while (std::chrono::steady_clock::now() < deadline) {
        if (interrupted()) {
            return;
        }

        // 向上取整：剩余不足 1ms 时 duration_cast 会截断成 0，sleep_for(0) 会退化成忙等
        const auto remaining = std::chrono::ceil<std::chrono::milliseconds>(deadline - std::chrono::steady_clock::now());
        std::this_thread::sleep_for(std::min(kInterruptibleSleepPollInterval, remaining));
    }
}

inline void sleep_until_interruptible(const std::chrono::steady_clock::time_point& deadline, const std::function<bool()>& interrupted)
{
    while (std::chrono::steady_clock::now() < deadline) {
        if (interrupted()) {
            return;
        }

        const auto remaining = std::chrono::ceil<std::chrono::milliseconds>(deadline - std::chrono::steady_clock::now());
        std::this_thread::sleep_for(std::min(kInterruptibleSleepPollInterval, remaining));
    }
}

MAA_TASK_NS_END
