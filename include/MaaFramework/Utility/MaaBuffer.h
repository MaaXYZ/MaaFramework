/**
 * @file MaaBuffer.h
 * @author
 * @brief Provide helpers funtions to manage, read and edit buffers.
 *
 * @copyright Copyright (c) 2024
 *
 */

// IWYU pragma: private, include <MaaFramework/MaaAPI.h>

#pragma once

#include "../MaaDef.h"
#include "../MaaPort.h"

#ifdef __cplusplus
extern "C"
{
#endif

    MAA_FRAMEWORK_API MaaStringBufferHandle MaaCreateStringBuffer();
    MAA_FRAMEWORK_API void MaaDestroyStringBuffer(MaaStringBufferHandle handle);
    MAA_FRAMEWORK_API MaaBool MaaIsStringEmpty(MaaStringBufferHandle handle);
    MAA_FRAMEWORK_API MaaBool MaaClearString(MaaStringBufferHandle handle);

    MAA_FRAMEWORK_API MaaStringView MaaGetString(MaaStringBufferHandle handle);
    MAA_FRAMEWORK_API MaaSize MaaGetStringSize(MaaStringBufferHandle handle);
    MAA_FRAMEWORK_API MaaBool MaaSetString(MaaStringBufferHandle handle, MaaStringView str);
    MAA_FRAMEWORK_API MaaBool
        MaaSetStringEx(MaaStringBufferHandle handle, MaaStringView str, MaaSize size);

    MAA_FRAMEWORK_API MaaStringListBufferHandle MaaCreateStringListBuffer();
    MAA_FRAMEWORK_API void MaaDestroyStringListBuffer(MaaStringListBufferHandle handle);
    MAA_FRAMEWORK_API MaaBool MaaIsStringListEmpty(MaaStringListBufferHandle handle);
    MAA_FRAMEWORK_API MaaBool MaaClearStringList(MaaStringListBufferHandle handle);
    MAA_FRAMEWORK_API MaaSize MaaGetStringListSize(MaaStringListBufferHandle handle);
    // return a string view, don't destory it
    MAA_FRAMEWORK_API MaaStringBufferHandle
        MaaGetStringListAt(MaaStringListBufferHandle handle, MaaSize index);
    // append a deep copy of value, you still need to destory it.
    MAA_FRAMEWORK_API MaaBool
        MaaStringListAppend(MaaStringListBufferHandle handle, MaaStringBufferHandle value);
    MAA_FRAMEWORK_API MaaBool MaaStringListRemove(MaaStringListBufferHandle handle, MaaSize index);

    MAA_FRAMEWORK_API MaaImageBufferHandle MaaCreateImageBuffer();
    MAA_FRAMEWORK_API void MaaDestroyImageBuffer(MaaImageBufferHandle handle);
    MAA_FRAMEWORK_API MaaBool MaaIsImageEmpty(MaaImageBufferHandle handle);
    MAA_FRAMEWORK_API MaaBool MaaClearImage(MaaImageBufferHandle handle);

    typedef void* MaaImageRawData;
    MAA_FRAMEWORK_API MaaImageRawData MaaGetImageRawData(MaaImageBufferHandle handle);
    MAA_FRAMEWORK_API int32_t MaaGetImageWidth(MaaImageBufferHandle handle);
    MAA_FRAMEWORK_API int32_t MaaGetImageHeight(MaaImageBufferHandle handle);
    MAA_FRAMEWORK_API int32_t MaaGetImageType(MaaImageBufferHandle handle);
    MAA_FRAMEWORK_API MaaBool MaaSetImageRawData(
        MaaImageBufferHandle handle,
        MaaImageRawData data,
        int32_t width,
        int32_t height,
        int32_t type);

    typedef uint8_t* MaaImageEncodedData;
    MAA_FRAMEWORK_API MaaImageEncodedData MaaGetImageEncoded(MaaImageBufferHandle handle);
    MAA_FRAMEWORK_API MaaSize MaaGetImageEncodedSize(MaaImageBufferHandle handle);
    MAA_FRAMEWORK_API MaaBool
        MaaSetImageEncoded(MaaImageBufferHandle handle, MaaImageEncodedData data, MaaSize size);

    MAA_FRAMEWORK_API MaaImageListBufferHandle MaaCreateImageListBuffer();
    MAA_FRAMEWORK_API void MaaDestroyImageListBuffer(MaaImageListBufferHandle handle);
    MAA_FRAMEWORK_API MaaBool MaaIsImageListEmpty(MaaImageListBufferHandle handle);
    MAA_FRAMEWORK_API MaaBool MaaClearImageList(MaaImageListBufferHandle handle);
    MAA_FRAMEWORK_API MaaSize MaaGetImageListSize(MaaImageListBufferHandle handle);
    // return an image view, don't destory it
    MAA_FRAMEWORK_API MaaImageBufferHandle
        MaaGetImageListAt(MaaImageListBufferHandle handle, MaaSize index);
    // append a deep copy of value, you still need to destory it.
    MAA_FRAMEWORK_API MaaBool
        MaaImageListAppend(MaaImageListBufferHandle handle, MaaImageBufferHandle value);
    MAA_FRAMEWORK_API MaaBool MaaImageListRemove(MaaImageListBufferHandle handle, MaaSize index);

    MAA_FRAMEWORK_API MaaRectHandle MaaCreateRectBuffer();
    MAA_FRAMEWORK_API void MaaDestroyRectBuffer(MaaRectHandle handle);
    MAA_FRAMEWORK_API int32_t MaaGetRectX(MaaRectHandle handle);
    MAA_FRAMEWORK_API int32_t MaaGetRectY(MaaRectHandle handle);
    MAA_FRAMEWORK_API int32_t MaaGetRectW(MaaRectHandle handle);
    MAA_FRAMEWORK_API int32_t MaaGetRectH(MaaRectHandle handle);

    MAA_FRAMEWORK_API MaaBool
        MaaSetRect(MaaRectHandle handle, int32_t x, int32_t y, int32_t w, int32_t h);
    MAA_FRAMEWORK_API MaaBool MaaSetRectX(MaaRectHandle handle, int32_t value);
    MAA_FRAMEWORK_API MaaBool MaaSetRectY(MaaRectHandle handle, int32_t value);
    MAA_FRAMEWORK_API MaaBool MaaSetRectW(MaaRectHandle handle, int32_t value);
    MAA_FRAMEWORK_API MaaBool MaaSetRectH(MaaRectHandle handle, int32_t value);

#ifdef __cplusplus
}
#endif
