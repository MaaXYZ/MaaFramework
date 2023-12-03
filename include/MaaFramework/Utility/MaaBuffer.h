#pragma once

#include "../MaaDef.h"
#include "../MaaPort.h"

#ifdef __cplusplus
extern "C"
{
#endif

    MaaStringBufferHandle MAA_FRAMEWORK_API MaaCreateStringBuffer();
    void MAA_FRAMEWORK_API MaaDestroyStringBuffer(MaaStringBufferHandle handle);
    MaaBool MAA_FRAMEWORK_API MaaIsStringEmpty(MaaStringBufferHandle handle);
    MaaBool MAA_FRAMEWORK_API MaaClearString(MaaStringBufferHandle handle);

    MaaStringView MAA_FRAMEWORK_API MaaGetString(MaaStringBufferHandle handle);
    MaaSize MAA_FRAMEWORK_API MaaGetStringSize(MaaStringBufferHandle handle);
    MaaBool MAA_FRAMEWORK_API MaaSetString(MaaStringBufferHandle handle, MaaStringView str);
    MaaBool MAA_FRAMEWORK_API MaaSetStringEx(MaaStringBufferHandle handle, MaaStringView str, MaaSize size);

    MaaImageBufferHandle MAA_FRAMEWORK_API MaaCreateImageBuffer();
    void MAA_FRAMEWORK_API MaaDestroyImageBuffer(MaaImageBufferHandle handle);
    MaaBool MAA_FRAMEWORK_API MaaIsImageEmpty(MaaImageBufferHandle handle);
    MaaBool MAA_FRAMEWORK_API MaaClearImage(MaaImageBufferHandle handle);

    typedef void* MaaImageRawData;
    MaaImageRawData MAA_FRAMEWORK_API MaaGetImageRawData(MaaImageBufferHandle handle);
    int32_t MAA_FRAMEWORK_API MaaGetImageWidth(MaaImageBufferHandle handle);
    int32_t MAA_FRAMEWORK_API MaaGetImageHeight(MaaImageBufferHandle handle);
    int32_t MAA_FRAMEWORK_API MaaGetImageType(MaaImageBufferHandle handle);
    MaaBool MAA_FRAMEWORK_API MaaSetImageRawData(MaaImageBufferHandle handle, MaaImageRawData data, int32_t width,
                                                 int32_t height, int32_t type);

    typedef uint8_t* MaaImageEncodedData;
    MaaImageEncodedData MAA_FRAMEWORK_API MaaGetImageEncoded(MaaImageBufferHandle handle);
    MaaSize MAA_FRAMEWORK_API MaaGetImageEncodedSize(MaaImageBufferHandle handle);
    MaaBool MAA_FRAMEWORK_API MaaSetImageEncoded(MaaImageBufferHandle handle, MaaImageEncodedData data, MaaSize size);

    MaaRectHandle MAA_FRAMEWORK_API MaaCreateRectBuffer();
    void MAA_FRAMEWORK_API MaaDestroyRectBuffer(MaaRectHandle handle);
    int32_t MAA_FRAMEWORK_API MaaGetRectX(MaaRectHandle handle);
    int32_t MAA_FRAMEWORK_API MaaGetRectY(MaaRectHandle handle);
    int32_t MAA_FRAMEWORK_API MaaGetRectW(MaaRectHandle handle);
    int32_t MAA_FRAMEWORK_API MaaGetRectH(MaaRectHandle handle);

    MaaBool MAA_FRAMEWORK_API MaaSetRect(MaaRectHandle handle, int32_t x, int32_t y, int32_t w, int32_t h);
    MaaBool MAA_FRAMEWORK_API MaaSetRectX(MaaRectHandle handle, int32_t value);
    MaaBool MAA_FRAMEWORK_API MaaSetRectY(MaaRectHandle handle, int32_t value);
    MaaBool MAA_FRAMEWORK_API MaaSetRectW(MaaRectHandle handle, int32_t value);
    MaaBool MAA_FRAMEWORK_API MaaSetRectH(MaaRectHandle handle, int32_t value);

#ifdef __cplusplus
}
#endif
