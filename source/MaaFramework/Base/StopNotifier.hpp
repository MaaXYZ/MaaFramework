#pragma once

#include <chrono>
#include <condition_variable>
#include <mutex>

#include "Conf/Conf.h"
#include "Utils/NonCopyable.hpp"

MAA_NS_BEGIN

class StopNotifier : public NonCopyable
{
public:
    bool sleep(unsigned ms);

    void stop();

private:
    bool needs_stop_ = false;
    std::mutex mtx_;
    std::condition_variable cond_;
};

inline bool StopNotifier::sleep(unsigned ms)
{
    if (needs_stop_) {
        return false;
    }

    auto timeout = std::chrono::system_clock::now() + std::chrono::milliseconds(ms);

    std::unique_lock<std::mutex> cond_lock(mtx_);

    if (cond_.wait_until(cond_lock, timeout, [this]() {
            return this->needs_stop_; //
        })) {
        return false;
    }
    return true;
}

inline void StopNotifier::stop()
{
    std::unique_lock<std::mutex> cond_lock(mtx_);

    needs_stop_ = true;

    cond_.notify_all();
}

MAA_NS_END
