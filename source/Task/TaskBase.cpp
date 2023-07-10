#include "TaskBase.h"

#include <chrono>

#include "MaaUtils/Logger.hpp"

MAA_TASK_NS_BEGIN

void TaskBase::sleep(unsigned ms) const
{
    sleep(std::chrono::milliseconds(ms));
}

void TaskBase::sleep(std::chrono::milliseconds ms) const
{
    if (need_exit()) {
        return;
    }

    using namespace std::chrono_literals;

    if (ms == 0ms) {
        std::this_thread::yield();
        return;
    }

    auto interval = std::min(ms, 5000ms);

    LogTrace << "ready to sleep" << ms << VAR(interval);

    for (auto sleep_time = interval; sleep_time <= ms && !need_exit(); sleep_time += interval) {
        std::this_thread::sleep_for(interval);
    }
    if (!need_exit()) {
        std::this_thread::sleep_for(ms % interval);
    }

    LogTrace << "end of sleep" << ms << VAR(interval);
}

MAA_TASK_NS_END
