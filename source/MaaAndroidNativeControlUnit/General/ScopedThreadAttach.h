#pragma once

#include "AndroidExternalLib.h"
#include "MaaUtils/Logger.h"

namespace AndroidNativeNS
{

class ScopedThreadAttach
{
public:
    explicit ScopedThreadAttach(const AndroidExternalFunctions* funcs)
        : funcs_(funcs)
    {
        if (!funcs_) {
            return;
        }

        env_ = funcs_->attach_thread();
        if (!env_) {
            LogError << "AttachThread returned nullptr";
        }
    }

    ScopedThreadAttach(const ScopedThreadAttach&) = delete;
    ScopedThreadAttach& operator=(const ScopedThreadAttach&) = delete;

    ~ScopedThreadAttach()
    {
        if (!funcs_ || !env_) {
            return;
        }

        int ret = funcs_->detach_thread(env_);
        if (ret != 0) {
            LogWarn << "DetachThread failed" << VAR(ret);
        }
    }

    bool attached() const { return env_ != nullptr; }
    void* env() const { return env_; }

private:
    const AndroidExternalFunctions* funcs_ = nullptr;
    void* env_ = nullptr;
};

} // namespace AndroidNativeNS
