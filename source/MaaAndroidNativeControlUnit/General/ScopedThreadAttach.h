#pragma once

#include "AndroidExternalLib.h"
#include "MaaUtils/Logger.h"

namespace AndroidNativeNS
{

class ScopedThreadAttach
{
public:
    explicit ScopedThreadAttach(const AndroidExternalLib* library)
        : library_(library)
    {
        if (!library_) {
            return;
        }

        env_ = library_->AttachThread();
        if (!env_) {
            LogError << "AttachThread returned nullptr";
        }
    }

    ScopedThreadAttach(const ScopedThreadAttach&) = delete;
    ScopedThreadAttach& operator=(const ScopedThreadAttach&) = delete;

    ~ScopedThreadAttach()
    {
        if (!library_ || !env_) {
            return;
        }

        int ret = library_->DetachThread(env_);
        if (ret != 0) {
            LogWarn << "DetachThread failed" << VAR(ret);
        }
    }

    bool attached() const { return env_ != nullptr; }
    void* env() const { return env_; }

private:
    const AndroidExternalLib* library_ = nullptr;
    void* env_ = nullptr;
};

} // namespace AndroidNativeNS
