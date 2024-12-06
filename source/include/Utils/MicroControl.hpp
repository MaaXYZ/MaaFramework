#pragma once

#include <chrono>
#include <functional>
#include <set>
#include <thread>

#include "Conf/Conf.h"
#include "ControlUnit/ControlUnitAPI.h"

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
    constexpr double kInterval = 10; // ms
    const std::chrono::milliseconds delay(static_cast<int>(kInterval));

    const double total_step = duration / kInterval;
    const double x_step_len = (x2 - x1) / total_step;
    const double y_step_len = (y2 - y1) / total_step;

    auto now = std::chrono::steady_clock::now();

    touch_down(x1, y1);

    for (int step = 1; step < total_step; ++step) {
        int mx = static_cast<int>(x1 + step * x_step_len);
        int my = static_cast<int>(y1 + step * y_step_len);

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

inline void micro_multi_swipe(
    const std::vector<ControlUnitAPI::SwipeParam>& swipes,
    std::function<void(int, int, int)> touch_down,
    std::function<void(int, int, int)> touch_move,
    std::function<void(int, int, int)> touch_up)
{
    constexpr double kInterval = 10; // ms
    const std::chrono::milliseconds delay(static_cast<int>(kInterval));

    struct Operating
    {
        double total_step = 0;
        double x_step_len = 0;
        double y_step_len = 0;
        int step = 0;
    };

    std::vector<Operating> operating(swipes.size());

    for (size_t i = 0; i < swipes.size(); ++i) {
        const ControlUnitAPI::SwipeParam& s = swipes.at(i);
        Operating& o = operating.at(i);
        o.total_step = s.duration / kInterval;
        o.x_step_len = (s.x2 - s.x1) / o.total_step;
        o.y_step_len = (s.y2 - s.y1) / o.total_step;
    }

    const auto starting = std::chrono::steady_clock::now();
    auto now = starting;

    size_t over_count = 0;
    while (over_count < swipes.size()) {
        int now_point = static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(now - starting).count());

        for (size_t i = 0; i < swipes.size(); ++i) {
            const ControlUnitAPI::SwipeParam& s = swipes.at(i);
            if (now_point < s.starting) {
                continue;
            }

            Operating& o = operating.at(i);
            int contact = static_cast<int>(i);

            if (o.step == 0) {
                touch_down(contact, s.x1, s.y1);
                ++o.step;
            }
            else if (o.step < o.total_step) {
                int mx = static_cast<int>(s.x1 + o.step * o.x_step_len);
                int my = static_cast<int>(s.y1 + o.step * o.y_step_len);
                touch_move(contact, mx, my);
                ++o.step;
            }
            else if (o.step == o.total_step) {
                touch_up(contact, s.x2, s.y2);
                ++o.step;
                ++over_count;
            }
            else { // step > total
                continue;
            }
        }

        std::this_thread::sleep_until(now + delay);
        now = std::chrono::steady_clock::now();
    }
}

MAA_CTRL_UNIT_NS_END
