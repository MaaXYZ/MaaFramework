#pragma once

#include <string>

#include <MaaFramework/MaaAPI.h>

namespace maa::details
{

class String
{
public:
    String()
        : handle_(MaaCreateStringBuffer())
        , own_(true)
    {
    }

    String(MaaStringBufferHandle handle)
        : handle_(handle)
        , own_(false)
    {
    }

    String(const String&) = delete;
    String& operator=(const String&) = delete;

    ~String()
    {
        if (own_) {
            MaaDestroyStringBuffer(handle_);
        }
    }

    MaaStringBufferHandle handle() const { return handle_; }

    std::string str() const
    {
        return std::string(MaaGetString(handle_), MaaGetStringSize(handle_));
    }

    operator std::string() const { return str(); }

    String& operator=(const std::string& str)
    {
        MaaSetStringEx(handle_, str.c_str(), str.size());
        return *this;
    }

private:
    MaaStringBufferHandle handle_ = nullptr;
    bool own_ = true;
};

}
