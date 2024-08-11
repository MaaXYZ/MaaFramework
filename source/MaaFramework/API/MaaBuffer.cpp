#include "MaaFramework/Utility/MaaBuffer.h"

#include "Buffer/ImageBuffer.hpp"
#include "Buffer/ListBuffer.hpp"
#include "Buffer/StringBuffer.hpp"
#include "Utils/Logger.h"

MaaStringBuffer* MaaStringBufferCreate()
{
    return new MAA_NS::StringBuffer;
}

void MaaStringBufferDestroy(MaaStringBuffer* handle)
{
    if (!handle) {
        LogError << "handle is null";
        return;
    }

    delete handle;
}

MaaBool MaaStringBufferIsEmpty(MaaStringBuffer* handle)
{
    if (!handle) {
        LogError << "handle is null";
        return true; // means empty
    }

    return handle->empty();
}

MaaBool MaaStringBufferClear(MaaStringBuffer* handle)
{
    if (!handle) {
        LogError << "handle is null";
        return false;
    }

    handle->clear();
    return true;
}

const char* MaaStringBufferGet(MaaStringBuffer* handle)
{
    if (!handle) {
        LogError << "handle is null";
        return nullptr;
    }

    return handle->data();
}

MaaSize MaaStringBufferSize(MaaStringBuffer* handle)
{
    if (!handle) {
        LogError << "handle is null";
        return 0;
    }

    return handle->size();
}

MaaBool MaaStringBufferSet(MaaStringBuffer* handle, const char* str)
{
    if (!handle || !str) {
        LogError << "handle is null";
        return false;
    }

    handle->set(str);
    return true;
}

MaaBool MaaStringBufferSetEx(MaaStringBuffer* handle, const char* str, MaaSize size)
{
    if (!handle || !str) {
        LogError << "handle is null";
        return false;
    }

    handle->set(std::string(str, size));
    return true;
}

MaaStringListBuffer* MaaStringListBufferCreate()
{
    return new MaaStringListBuffer;
}

void MaaStringListBufferDestroy(MaaStringListBuffer* handle)
{
    if (!handle) {
        LogError << "handle is null";
        return;
    }

    delete handle;
}

MaaBool MaaStringListBufferIsEmpty(MaaStringListBuffer* handle)
{
    if (!handle) {
        LogError << "handle is null";
        return true; // means empty
    }

    return handle->empty();
}

MaaBool MaaStringListBufferClear(MaaStringListBuffer* handle)
{
    if (!handle) {
        LogError << "handle is null";
        return false;
    }

    handle->clear();
    return true;
}

MaaSize MaaStringListBufferSize(MaaStringListBuffer* handle)
{
    if (!handle) {
        LogError << "handle is null";
        return 0;
    }

    return handle->size();
}

MaaStringBuffer* MaaStringListBufferAt(MaaStringListBuffer* handle, MaaSize index)
{
    if (!handle) {
        LogError << "handle is null";
        return nullptr;
    }

    return &(handle->at(index));
}

MaaBool MaaStringListBufferAppend(MaaStringListBuffer* handle, MaaStringBuffer* value)
{
    if (!handle || !value) {
        LogError << "handle is null";
        return false;
    }

    handle->append(dynamic_cast<const MaaNS::StringBuffer&>(*value));
    return true;
}

MaaBool MaaStringListBufferRemove(MaaStringListBuffer* handle, MaaSize index)
{
    if (!handle) {
        LogError << "handle is null";
        return false;
    }

    handle->remove(index);
    return true;
}

MaaImageBuffer* MaaImageBufferCreate()
{
    return new MAA_NS::ImageBuffer;
}

void MaaImageBufferDestroy(MaaImageBuffer* handle)
{
    if (!handle) {
        LogError << "handle is null";
        return;
    }

    delete handle;
}

MaaBool MaaImageBufferIsEmpty(MaaImageBuffer* handle)
{
    if (!handle) {
        LogError << "handle is null";
        return true; // means empty
    }

    return handle->empty();
}

MaaBool MaaImageBufferClear(MaaImageBuffer* handle)
{
    if (!handle) {
        LogError << "handle is null";
        return false;
    }

    handle->clear();
    return true;
}

MaaImageRawData MaaImageBufferGetRawData(MaaImageBuffer* handle)
{
    if (!handle) {
        LogError << "handle is null";
        return nullptr;
    }

    return handle->raw_data();
}

int32_t MaaImageBufferWidth(MaaImageBuffer* handle)
{
    if (!handle) {
        LogError << "handle is null";
        return 0;
    }

    return handle->width();
}

int32_t MaaImageBufferHeight(MaaImageBuffer* handle)
{
    if (!handle) {
        LogError << "handle is null";
        return 0;
    }

    return handle->height();
}

int32_t MaaImageBufferType(MaaImageBuffer* handle)
{
    if (!handle) {
        LogError << "handle is null";
        return 0;
    }

    return handle->type();
}

MaaBool MaaImageBufferSetRawData(MaaImageBuffer* handle, MaaImageRawData data, int32_t width, int32_t height, int32_t type)
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

MaaImageEncodedData MaaImageBufferGetEncoded(MaaImageBuffer* handle)
{
    if (!handle) {
        LogError << "handle is null";
        return nullptr;
    }

    return handle->encoded();
}

MaaSize MaaImageBufferGetEncodedSize(MaaImageBuffer* handle)
{
    if (!handle) {
        LogError << "handle is null";
        return 0;
    }

    return handle->encoded_size();
}

MaaBool MaaImageBufferSetEncoded(MaaImageBuffer* handle, MaaImageEncodedData data, MaaSize size)
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

MaaImageListBuffer* MaaImageListBufferCreate()
{
    return new MaaImageListBuffer;
}

void MaaImageListBufferDestroy(MaaImageListBuffer* handle)
{
    if (!handle) {
        LogError << "handle is null";
        return;
    }

    delete handle;
}

MaaBool MaaImageListBufferIsEmpty(MaaImageListBuffer* handle)
{
    if (!handle) {
        LogError << "handle is null";
        return true; // means empty
    }

    return handle->empty();
}

MaaBool MaaImageListBufferClear(MaaImageListBuffer* handle)
{
    if (!handle) {
        LogError << "handle is null";
        return false;
    }

    handle->clear();
    return true;
}

MaaSize MaaImageListBufferSize(MaaImageListBuffer* handle)
{
    if (!handle) {
        LogError << "handle is null";
        return 0;
    }

    return handle->size();
}

MaaImageBuffer* MaaImageListBufferAt(MaaImageListBuffer* handle, MaaSize index)
{
    if (!handle) {
        LogError << "handle is null";
        return nullptr;
    }

    return &(handle->at(index));
}

MaaBool MaaImageListBufferAppend(MaaImageListBuffer* handle, MaaImageBuffer* value)
{
    if (!handle || !value) {
        LogError << "handle is null";
        return false;
    }

    handle->append(dynamic_cast<const MaaNS::ImageBuffer&>(*value));
    return true;
}

MaaBool MaaImageListBufferRemove(MaaImageListBuffer* handle, MaaSize index)
{
    if (!handle) {
        LogError << "handle is null";
        return false;
    }

    handle->remove(index);
    return true;
}

MaaRect* MaaRectBufferCreate()
{
    return new MaaRect {};
}

void MaaRectBufferDestroy(MaaRect* handle)
{
    if (!handle) {
        LogError << "handle is null";
        return;
    }

    delete handle;
}

int32_t MaaRectGetX(MaaRect* handle)
{
    if (!handle) {
        LogError << "handle is null";
        return 0;
    }

    return handle->x;
}

int32_t MaaRectGetY(MaaRect* handle)
{
    if (!handle) {
        LogError << "handle is null";
        return 0;
    }

    return handle->y;
}

int32_t MaaRectGetW(MaaRect* handle)
{
    if (!handle) {
        LogError << "handle is null";
        return 0;
    }

    return handle->width;
}

int32_t MaaRectGetH(MaaRect* handle)
{
    if (!handle) {
        LogError << "handle is null";
        return 0;
    }

    return handle->height;
}

MaaBool MaaRectSet(MaaRect* handle, int32_t x, int32_t y, int32_t w, int32_t h)
{
    if (!handle) {
        LogError << "handle is null";
        return false;
    }

    handle->x = x;
    handle->y = y;
    handle->width = w;
    handle->height = h;
    return true;
}
