#pragma once

#include <chrono>
#include <functional>
#include <thread>

#include "Conf/Conf.h"

MAA_CTRL_UNIT_NS_BEGIN

inline void micro_swipe(
    int x1,
    int y1,
    int x2,
    int y2,
    int duration,
    std::function<void(int, int)> touch_down,
    std::function<void(int, int)> touch_move,
    std::function<void(int, int)> touch_up)
{
    auto now = std::chrono::steady_clock::now();

    touch_down(x1, y1);

    constexpr double kInterval = 10; // ms
    const double steps = duration / kInterval;
    const double x_step_len = (x2 - x1) / steps;
    const double y_step_len = (y2 - y1) / steps;
    const std::chrono::milliseconds delay(static_cast<int>(kInterval));

    for (int i = 0; i < steps; ++i) {
        int mx = static_cast<int>(x1 + i * x_step_len);
        int my = static_cast<int>(y1 + i * y_step_len);

        std::this_thread::sleep_until(now + delay);

        now = std::chrono::steady_clock::now();
        touch_move(mx, my);
    }

    std::this_thread::sleep_until(now + delay);

    now = std::chrono::steady_clock::now();
    touch_move(x2, y2);

    std::this_thread::sleep_until(now + delay);

    now = std::chrono::steady_clock::now();
    touch_up(x2, y2);
}

MAA_CTRL_UNIT_NS_END
