#pragma once

#include <vector>

#include <MaaFramework/MaaAPI.h>

#include "../Exception.h"

namespace maapp::pri
{

struct Image
{
    Image() { buffer_ = MaaImageBufferCreate(); }

    Image(const Image&) = delete;

    Image(Image&& str)
    {
        buffer_ = str.buffer_;
        str.buffer_ = nullptr;
    }

    ~Image()
    {
        if (buffer_) {
            MaaImageBufferDestroy(buffer_);
        }
    }

    Image& operator=(const Image&) = delete;

    Image& operator=(Image&& str)
    {
        if (&str == this) {
            return *this;
        }
        if (buffer_) {
            MaaImageBufferDestroy(buffer_);
        }
        buffer_ = str.buffer_;
        str.buffer_ = nullptr;
        return *this;
    }

    Image& operator=(const std::vector<uint8_t>& data)
    {
        if (!MaaImageBufferSetEncoded(buffer_, const_cast<uint8_t*>(data.data()), data.size())) {
            throw FunctionFailed("MaaImageBufferSetEncoded");
        }
        return *this;
    }

    operator std::vector<uint8_t>() const
    {
        auto data = MaaImageBufferGetEncoded(buffer_);
        return std::vector<uint8_t> {
            data,
            data + MaaImageBufferGetEncodedSize(buffer_),
        };
    }

    MaaImageBuffer* buffer_ {};
};

struct ImageList
{
    ImageList() { buffer_ = MaaImageListBufferCreate(); }

    ImageList(const ImageList&) = delete;

    ImageList(ImageList&& str)
    {
        buffer_ = str.buffer_;
        str.buffer_ = nullptr;
    }

    ~ImageList()
    {
        if (buffer_) {
            MaaImageListBufferDestroy(buffer_);
        }
    }

    ImageList& operator=(const ImageList&) = delete;

    ImageList& operator=(ImageList&& str)
    {
        if (&str == this) {
            return *this;
        }
        if (buffer_) {
            MaaImageListBufferDestroy(buffer_);
        }
        buffer_ = str.buffer_;
        str.buffer_ = nullptr;
        return *this;
    }

    ImageList& operator=(const std::vector<std::vector<uint8_t>>& data)
    {
        if (!MaaImageListBufferClear(buffer_)) {
            throw FunctionFailed("MaaImageListBufferClear");
        }
        for (const auto& img_data : data) {
            Image image;
            image = img_data;
            MaaImageListBufferAppend(buffer_, image.buffer_);
        }
        return *this;
    }

    operator std::vector<std::vector<uint8_t>>() const
    {
        size_t count = MaaImageListBufferSize(buffer_);
        std::vector<std::vector<uint8_t>> result(count);

        for (size_t i = 0; i < count; i++) {
            auto image = MaaImageListBufferAt(buffer_, i);
            auto data = MaaImageBufferGetEncoded(image);
            result.push_back({
                data,
                data + MaaImageBufferGetEncodedSize(image),
            });
        }

        return result;
    }

    MaaImageListBuffer* buffer_ {};
};

}
