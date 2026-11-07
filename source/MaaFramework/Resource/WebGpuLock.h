#pragma once

#include <mutex>

#include "Common/Conf.h"

MAA_RES_NS_BEGIN

// ORT WebGPU shares encoder state across sessions. Serialize creation, execution
// and destruction, including cleanup of a model during a locked load (#1591).
inline std::unique_lock<std::recursive_mutex> lock_webgpu(bool enabled)
{
    static std::recursive_mutex mutex;
    std::unique_lock lock(mutex, std::defer_lock);
    if (enabled) {
        lock.lock();
    }
    return lock;
}

struct WebGpuDeleter
{
    bool enabled = false;

    template <typename T>
    void operator()(T* object) const
    {
        auto lock = lock_webgpu(enabled);
        delete object;
    }
};

MAA_RES_NS_END
