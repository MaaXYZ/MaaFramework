// IWYU pragma: private, include <MaaPP/MaaPP.hpp>

#pragma once

#include <string>

#include <MaaFramework/MaaAPI.h>

#include "MaaPP/maa/Exception.hpp"

namespace maa::details
{

class String
{
public:
    String()
        : String(MaaCreateStringBuffer(), true)
    {
    }

    String(MaaStringBufferHandle handle, bool own = false)
        : handle_(handle)
        , own_(own)
    {
        if (!handle_) {
            throw NullHandle<String, MaaStringBufferHandle>();
        }
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
