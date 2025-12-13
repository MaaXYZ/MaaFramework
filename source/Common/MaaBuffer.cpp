#include "MaaFramework/Utility/MaaBuffer.h"

#include "MaaUtils/Buffer/ImageBuffer.hpp"
#include "MaaUtils/Buffer/ListBuffer.hpp"
#include "MaaUtils/Buffer/StringBuffer.hpp"
#include "MaaUtils/Logger.h"

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

MaaBool MaaStringBufferIsEmpty(const MaaStringBuffer* handle)
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

const char* MaaStringBufferGet(const MaaStringBuffer* handle)
{
    if (!handle) {
        LogError << "handle is null";
        return "";
    }

    return handle->data();
}

MaaSize MaaStringBufferSize(const MaaStringBuffer* handle)
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

MaaBool MaaStringListBufferIsEmpty(const MaaStringListBuffer* handle)
{
    if (!handle) {
        LogError << "handle is null";
        return true; // means empty
    }

    return handle->empty();
}

MaaSize MaaStringListBufferSize(const MaaStringListBuffer* handle)
{
    if (!handle) {
        LogError << "handle is null";
        return 0;
    }

    return handle->size();
}

const MaaStringBuffer* MaaStringListBufferAt(const MaaStringListBuffer* handle, MaaSize index)
{
    if (!handle) {
        LogError << "handle is null";
        return nullptr;
    }
    if (index >= handle->size()) {
        LogError << "out of range" << VAR(index) << VAR(handle->size());
        return nullptr;
    }

    return &(handle->at(index));
}

MaaBool MaaStringListBufferAppend(MaaStringListBuffer* handle, const MaaStringBuffer* value)
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
    if (index >= handle->size()) {
        LogError << "out of range" << VAR(index) << VAR(handle->size());
        return false;
    }

    handle->remove(index);
    return true;
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

MaaBool MaaImageBufferIsEmpty(const MaaImageBuffer* handle)
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

MaaImageRawData MaaImageBufferGetRawData(const MaaImageBuffer* handle)
{
    if (!handle) {
        LogError << "handle is null";
        return nullptr;
    }

    return handle->raw_data();
}

int32_t MaaImageBufferWidth(const MaaImageBuffer* handle)
{
    if (!handle) {
        LogError << "handle is null";
        return 0;
    }

    return handle->width();
}

int32_t MaaImageBufferHeight(const MaaImageBuffer* handle)
{
    if (!handle) {
        LogError << "handle is null";
        return 0;
    }

    return handle->height();
}

int32_t MaaImageBufferChannels(const MaaImageBuffer* handle)
{
    if (!handle) {
        LogError << "handle is null";
        return 0;
    }

    return handle->channels();
}

int32_t MaaImageBufferType(const MaaImageBuffer* handle)
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

MaaImageEncodedData MaaImageBufferGetEncoded(const MaaImageBuffer* handle)
{
    if (!handle) {
        LogError << "handle is null";
        return nullptr;
    }

    return handle->encoded();
}

MaaSize MaaImageBufferGetEncodedSize(const MaaImageBuffer* handle)
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

    if (!size) {
        handle->clear();
        return true;
    }

    ImageEncodedBuffer buffer(data, data + size);
    handle->set(std::move(buffer));

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

MaaBool MaaImageListBufferIsEmpty(const MaaImageListBuffer* handle)
{
    if (!handle) {
        LogError << "handle is null";
        return true; // means empty
    }

    return handle->empty();
}

MaaSize MaaImageListBufferSize(const MaaImageListBuffer* handle)
{
    if (!handle) {
        LogError << "handle is null";
        return 0;
    }

    return handle->size();
}

const MaaImageBuffer* MaaImageListBufferAt(const MaaImageListBuffer* handle, MaaSize index)
{
    if (!handle) {
        LogError << "handle is null";
        return nullptr;
    }
    if (index >= handle->size()) {
        LogError << "out of range" << VAR(index) << VAR(handle->size());
        return nullptr;
    }

    return &(handle->at(index));
}

MaaBool MaaImageListBufferAppend(MaaImageListBuffer* handle, const MaaImageBuffer* value)
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
    if (index >= handle->size()) {
        LogError << "out of range" << VAR(index) << VAR(handle->size());
        return false;
    }

    handle->remove(index);
    return true;
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

MaaRect* MaaRectCreate()
{
    return new MaaRect {};
}

void MaaRectDestroy(MaaRect* handle)
{
    if (!handle) {
        LogError << "handle is null";
        return;
    }

    delete handle;
}

int32_t MaaRectGetX(const MaaRect* handle)
{
    if (!handle) {
        LogError << "handle is null";
        return 0;
    }

    return handle->x;
}

int32_t MaaRectGetY(const MaaRect* handle)
{
    if (!handle) {
        LogError << "handle is null";
        return 0;
    }

    return handle->y;
}

int32_t MaaRectGetW(const MaaRect* handle)
{
    if (!handle) {
        LogError << "handle is null";
        return 0;
    }

    return handle->width;
}

int32_t MaaRectGetH(const MaaRect* handle)
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
