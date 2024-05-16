// IWYU pragma: private, include <MaaPP/MaaPP.hpp>

#pragma once

#include <memory>
#include <string_view>

#include <MaaFramework/MaaAPI.h>

#include "MaaPP/maa/Exception.hpp"

namespace maa::details
{

class Image
{
public:
    template <typename... Args>
    static auto make(Args&&... args)
    {
        return std::make_shared<Image>(std::forward<Args>(args)...);
    }

    Image()
        : Image(MaaCreateImageBuffer(), true)
    {
    }

    Image(MaaImageBufferHandle handle, bool own = false)
        : handle_(handle)
        , own_(own)
    {
        if (!handle_) {
            throw NullHandle<Image, MaaImageBufferHandle>();
        }
    }

    Image(const Image&) = delete;
    Image& operator=(const Image&) = delete;

    ~Image()
    {
        if (own_) {
            MaaDestroyImageBuffer(handle_);
        }
    }

    MaaImageBufferHandle handle() const { return handle_; }

    bool empty() const { return MaaIsImageEmpty(handle_); }

    bool clear() const { return MaaClearImage(handle_); }

    int32_t width() const { return MaaGetImageWidth(handle_); }

    int32_t height() const { return MaaGetImageHeight(handle_); }

    int32_t type() const { return MaaGetImageType(handle_); }

    std::string_view encoded()
    {
        return std::string_view(
            reinterpret_cast<char*>(MaaGetImageEncoded(handle_)),
            MaaGetImageEncodedSize(handle_));
    }

    void set_encoded(std::string_view data)
    {
        if (!MaaSetImageEncoded(
                handle_,
                const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(data.data())),
                data.size())) {
            throw FunctionFailed("MaaSetImageEncoded");
        }
    }

private:
    MaaImageBufferHandle handle_ = nullptr;
    bool own_ = true;
};

class ImageList
{
public:
    ImageList()
        : handle_(MaaCreateImageListBuffer())
        , own_(true)
    {
    }

    ImageList(MaaImageListBufferHandle handle)
        : handle_(handle)
        , own_(false)
    {
    }

    ImageList(const ImageList&) = delete;
    ImageList& operator=(const ImageList&) = delete;

    ~ImageList()
    {
        if (own_) {
            MaaDestroyImageListBuffer(handle_);
        }
    }

    MaaImageListBufferHandle handle() const { return handle_; }

    bool empty() const { return MaaIsImageListEmpty(handle_); }

    bool clear() const { return MaaClearImageList(handle_); }

    size_t size() const { return MaaGetImageListSize(handle_); }

    Image get(size_t index) const { return Image(MaaGetImageListAt(handle_, index)); }

    bool append(const Image& img) const { return MaaImageListAppend(handle_, img.handle()); }

    bool remove(size_t index) const { return MaaImageListRemove(handle_, index); }

private:
    MaaImageListBufferHandle handle_ = nullptr;
    bool own_ = true;
};

}
