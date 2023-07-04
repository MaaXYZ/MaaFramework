#include "TaskBase.h"

#include "MaaUtils/Logger.hpp"

MAA_TASK_NS_BEGIN

void TaskBase::sleep(uint ms) const
{
    if (need_exit()) {
        return;
    }
    if (ms == 0) {
        std::this_thread::yield();
        return;
    }

    auto millisecond = std::chrono::milliseconds(ms);
    auto interval = std::chrono::milliseconds(std::min(ms, 5000U));

    LogTrace << "ready to sleep" << millisecond << VAR(interval);

    for (auto sleep_time = interval; sleep_time <= millisecond && !need_exit(); sleep_time += interval) {
        std::this_thread::sleep_for(interval);
    }
    if (!need_exit()) {
        std::this_thread::sleep_for(millisecond % interval);
    }

    LogTrace << "end of sleep" << millisecond << VAR(interval);
}

MAA_TASK_NS_END
