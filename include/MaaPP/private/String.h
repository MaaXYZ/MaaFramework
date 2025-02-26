#pragma once

#include <string>
#include <vector>

#include <MaaFramework/MaaAPI.h>

#include "../Exception.h"

namespace maapp::pri
{

struct String
{
    String() { buffer_ = MaaStringBufferCreate(); }

    String(const String&) = delete;

    String(String&& str)
    {
        buffer_ = str.buffer_;
        str.buffer_ = nullptr;
    }

    ~String()
    {
        if (buffer_) {
            MaaStringBufferDestroy(buffer_);
        }
    }

    String& operator=(const String&) = delete;

    String& operator=(String&& str)
    {
        if (&str == this) {
            return *this;
        }
        if (buffer_) {
            MaaStringBufferDestroy(buffer_);
        }
        buffer_ = str.buffer_;
        str.buffer_ = nullptr;
        return *this;
    }

    String& operator=(const std::string& str)
    {
        if (!MaaStringBufferSetEx(buffer_, str.c_str(), str.length())) {
            throw FunctionFailed("MaaStringBufferSetEx");
        }
        return *this;
    }

    operator std::string() const
    {
        return std::string {
            MaaStringBufferGet(buffer_),
            MaaStringBufferSize(buffer_),
        };
    }

    MaaStringBuffer* buffer_ {};
};

struct StringList
{
    StringList() { buffer_ = MaaStringListBufferCreate(); }

    StringList(const StringList&) = delete;

    StringList(StringList&& str)
    {
        buffer_ = str.buffer_;
        str.buffer_ = nullptr;
    }

    ~StringList()
    {
        if (buffer_) {
            MaaStringListBufferDestroy(buffer_);
        }
    }

    StringList& operator=(const StringList&) = delete;

    StringList& operator=(StringList&& str)
    {
        if (&str == this) {
            return *this;
        }
        if (buffer_) {
            MaaStringListBufferDestroy(buffer_);
        }
        buffer_ = str.buffer_;
        str.buffer_ = nullptr;
        return *this;
    }

    StringList& operator=(const std::vector<std::string>& data)
    {
        if (!MaaStringListBufferClear(buffer_)) {
            throw FunctionFailed("MaaStringListBufferClear");
        }
        for (const auto& str_data : data) {
            String str;
            str = str_data;
            MaaStringListBufferAppend(buffer_, str.buffer_);
        }
        return *this;
    }

    operator std::vector<std::string>() const
    {
        size_t count = MaaStringListBufferSize(buffer_);
        std::vector<std::string> result(count);

        for (size_t i = 0; i < count; i++) {
            auto image = MaaStringListBufferAt(buffer_, i);
            result.push_back({
                MaaStringBufferGet(image),
                MaaStringBufferSize(image),
            });
        }

        return result;
    }

    MaaStringListBuffer* buffer_ {};
};
}
