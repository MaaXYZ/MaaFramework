#pragma once

#include <memory>
#include <mutex>
#include <utility>

#include "Common/Conf.h"

MAA_RES_NS_BEGIN

// GPU runtimes may share state across sessions (#1591). Use one lock for all
// backends and resources, including model creation and final release.
class InferenceContext
{
public:
    InferenceContext() = default;

    explicit InferenceContext(bool use_gpu)
        : use_gpu_(use_gpu)
    {
    }

    [[nodiscard]] std::unique_lock<std::recursive_mutex> lock() const
    {
        if (!use_gpu_) {
            return { };
        }

        // Loading or clearing resources can release models while already locked.
        static std::recursive_mutex mutex;
        return std::unique_lock(mutex);
    }

    template <typename T, typename... Args>
    std::shared_ptr<T> make_shared(Args&&... args) const
    {
        auto guard = lock();
        return std::shared_ptr<T>(new T(std::forward<Args>(args)...), [context = *this](T* object) {
            auto guard = context.lock();
            delete object;
        });
    }

private:
    bool use_gpu_ = false;
};

MAA_RES_NS_END
