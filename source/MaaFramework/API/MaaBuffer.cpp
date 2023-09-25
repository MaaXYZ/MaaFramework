#include "MaaFramework/Utility/MaaBuffer.h"

#include "Buffer/ImageBuffer.hpp"
#include "Buffer/StringBuffer.hpp"
#include "Utils/Logger.h"

MaaStringBufferHandle MaaCreateStringBuffer()
{
    return new MAA_NS::StringBuffer;
}

void MaaDestroyStringBuffer(MaaStringBufferHandle handle)
{
    if (!handle) {
        LogError << "handle is null";
        return;
    }

    delete handle;
}

MaaBool MaaIsStringEmpty(MaaStringBufferHandle handle)
{
    if (!handle) {
        LogError << "handle is null";
        return true; // means empty
    }

    return handle->empty();
}

MaaBool MaaClearString(MaaStringBufferHandle handle)
{
    if (!handle) {
        LogError << "handle is null";
        return false;
    }

    handle->clear();
    return true;
}

MaaStringView MaaGetString(MaaStringBufferHandle handle)
{
    if (!handle) {
        LogError << "handle is null";
        return nullptr;
    }

    return handle->data();
}

MaaSize MaaGetStringSize(MaaStringBufferHandle handle)
{
    if (!handle) {
        LogError << "handle is null";
        return 0;
    }

    return handle->size();
}

MaaBool MaaSetString(MaaStringBufferHandle handle, MaaStringView str)
{
    if (!handle || !str) {
        LogError << "handle is null";
        return false;
    }

    handle->set(str);
    return true;
}

MaaBool MaaSetStringEx(MaaStringBufferHandle handle, MaaStringView str, MaaSize size)
{
    if (!handle || !str) {
        LogError << "handle is null";
        return false;
    }

    handle->set(std::string(str, size));
    return true;
}

MaaImageBufferHandle MaaCreateImageBuffer()
{
    return new MAA_NS::ImageBuffer;
}

void MaaDestroyImageBuffer(MaaImageBufferHandle handle)
{
    if (!handle) {
        LogError << "handle is null";
        return;
    }

    delete handle;
}

void* MaaGetImageRawData(MaaImageBufferHandle handle)
{
    if (!handle) {
        LogError << "handle is null";
        return nullptr;
    }

    return handle->raw_data();
}

int32_t MaaGetImageWidth(MaaImageBufferHandle handle)
{
    if (!handle) {
        LogError << "handle is null";
        return 0;
    }

    return handle->width();
}

int32_t MaaGetImageHeight(MaaImageBufferHandle handle)
{
    if (!handle) {
        LogError << "handle is null";
        return 0;
    }

    return handle->height();
}

int32_t MaaGetImageType(MaaImageBufferHandle handle)
{
    if (!handle) {
        LogError << "handle is null";
        return 0;
    }

    return handle->type();
}

MaaBool MaaIsImageEmpty(MaaImageBufferHandle handle)
{
    if (!handle) {
        LogError << "handle is null";
        return true; // means empty
    }

    return handle->empty();
}

MaaBool MaaClearImage(MaaImageBufferHandle handle)
{
    if (!handle) {
        LogError << "handle is null";
        return false;
    }

    handle->clear();
    return true;
}

MaaBool MaaSetImageRawData(MaaImageBufferHandle handle, MaaImageRawData data, int32_t width, int32_t height,
                           int32_t type)
{
    if (!handle || !data) {
        LogError << "handle is null";
        return false;
    }

    cv::Mat img(height, width, type, data);
    if (img.empty()) {
        LogError << "img is empty" << VAR_VOIDP(data) << VAR(width) << VAR(height) << VAR(type);
        return false;
    }

    handle->set(img);
    return true;
}

uint8_t* MaaGetImageEncoded(MaaImageBufferHandle handle)
{
    if (!handle) {
        LogError << "handle is null";
        return nullptr;
    }

    return handle->encoded();
}

MaaSize MaaGetImageEncodedSize(MaaImageBufferHandle handle)
{
    if (!handle) {
        LogError << "handle is null";
        return 0;
    }

    return handle->encoded_size();
}

MaaBool MaaSetImageEncoded(MaaImageBufferHandle handle, MaaImageEncodedData data, MaaSize size)
{
    if (!handle || !data) {
        LogError << "handle is null";
        return false;
    }

    cv::Mat img = cv::imdecode({ data, static_cast<int>(size) }, cv::IMREAD_COLOR);
    if (img.empty()) {
        LogError << "img is empty" << VAR_VOIDP(data) << VAR(size);
        return false;
    }

    handle->set(img);
    return true;
}

MaaRectHandle MaaCreateRectBuffer()
{
    return new MaaRect {};
}

void MaaDestroyRectBuffer(MaaRectHandle handle)
{
    if (!handle) {
        LogError << "handle is null";
        return;
    }

    delete handle;
}

int32_t MaaGetRectX(MaaRectHandle handle)
{
    if (!handle) {
        LogError << "handle is null";
        return 0;
    }

    return handle->x;
}

int32_t MaaGetRectY(MaaRectHandle handle)
{
    if (!handle) {
        LogError << "handle is null";
        return 0;
    }

    return handle->y;
}

int32_t MaaGetRectW(MaaRectHandle handle)
{
    if (!handle) {
        LogError << "handle is null";
        return 0;
    }

    return handle->width;
}

int32_t MaaGetRectH(MaaRectHandle handle)
{
    if (!handle) {
        LogError << "handle is null";
        return 0;
    }

    return handle->height;
}

MaaBool MaaSetRectX(MaaRectHandle handle, int32_t value)
{
    if (!handle) {
        LogError << "handle is null";
        return 0;
    }

    handle->x = value;
    return true;
}

MaaBool MaaSetRectY(MaaRectHandle handle, int32_t value)
{
    if (!handle) {
        LogError << "handle is null";
        return 0;
    }

    handle->y = value;
    return true;
}

MaaBool MaaSetRectW(MaaRectHandle handle, int32_t value)
{
    if (!handle) {
        LogError << "handle is null";
        return 0;
    }

    handle->width = value;
    return true;
}

MaaBool MaaSetRectH(MaaRectHandle handle, int32_t value)
{
    if (!handle) {
        LogError << "handle is null";
        return 0;
    }

    handle->height = value;
    return true;
}
