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

    MAA_FRAMEWORK_API MaaStringBuffer* MaaStringBufferCreate();
    MAA_FRAMEWORK_API void MaaStringBufferDestroy(MaaStringBuffer* handle);
    MAA_FRAMEWORK_API MaaBool MaaStringBufferIsEmpty(MaaStringBuffer* handle);
    MAA_FRAMEWORK_API MaaBool MaaStringBufferClear(MaaStringBuffer* handle);

    MAA_FRAMEWORK_API const char* MaaStringBufferGet(MaaStringBuffer* handle);
    MAA_FRAMEWORK_API MaaSize MaaStringBufferSize(MaaStringBuffer* handle);
    MAA_FRAMEWORK_API MaaBool MaaStringBufferSet(MaaStringBuffer* handle, const char* str);
    MAA_FRAMEWORK_API MaaBool MaaStringBufferSetEx(MaaStringBuffer* handle, const char* str, MaaSize size);

    MAA_FRAMEWORK_API MaaStringListBuffer* MaaStringListBufferCreate();
    MAA_FRAMEWORK_API void MaaStringListBufferDestroy(MaaStringListBuffer* handle);
    MAA_FRAMEWORK_API MaaBool MaaStringListBufferIsEmpty(MaaStringListBuffer* handle);
    MAA_FRAMEWORK_API MaaBool MaaStringListBufferClear(MaaStringListBuffer* handle);
    MAA_FRAMEWORK_API MaaSize MaaStringListBufferSize(MaaStringListBuffer* handle);
    // return a string view, don't destory it
    MAA_FRAMEWORK_API MaaStringBuffer* MaaStringListBufferAt(MaaStringListBuffer* handle, MaaSize index);
    // append a deep copy of value, you still need to destory it.
    MAA_FRAMEWORK_API MaaBool MaaStringListBufferAppend(MaaStringListBuffer* handle, MaaStringBuffer* value);
    MAA_FRAMEWORK_API MaaBool MaaStringListBufferRemove(MaaStringListBuffer* handle, MaaSize index);

    MAA_FRAMEWORK_API MaaImageBuffer* MaaImageBufferCreate();
    MAA_FRAMEWORK_API void MaaImageBufferDestroy(MaaImageBuffer* handle);
    MAA_FRAMEWORK_API MaaBool MaaImageBufferIsEmpty(MaaImageBuffer* handle);
    MAA_FRAMEWORK_API MaaBool MaaImageBufferClear(MaaImageBuffer* handle);

    typedef void* MaaImageRawData;
    MAA_FRAMEWORK_API MaaImageRawData MaaImageBufferGetRawData(MaaImageBuffer* handle);
    MAA_FRAMEWORK_API int32_t MaaImageBufferWidth(MaaImageBuffer* handle);
    MAA_FRAMEWORK_API int32_t MaaImageBufferHeight(MaaImageBuffer* handle);
    MAA_FRAMEWORK_API int32_t MaaImageBufferType(MaaImageBuffer* handle);
    MAA_FRAMEWORK_API MaaBool
        MaaImageBufferSetRawData(MaaImageBuffer* handle, MaaImageRawData data, int32_t width, int32_t height, int32_t type);

    typedef uint8_t* MaaImageEncodedData;
    MAA_FRAMEWORK_API MaaImageEncodedData MaaImageBufferGetEncoded(MaaImageBuffer* handle);
    MAA_FRAMEWORK_API MaaSize MaaImageBufferGetEncodedSize(MaaImageBuffer* handle);
    MAA_FRAMEWORK_API MaaBool MaaImageBufferSetEncoded(MaaImageBuffer* handle, MaaImageEncodedData data, MaaSize size);

    MAA_FRAMEWORK_API MaaImageListBuffer* MaaImageListBufferCreate();
    MAA_FRAMEWORK_API void MaaImageListBufferDestroy(MaaImageListBuffer* handle);
    MAA_FRAMEWORK_API MaaBool MaaImageListBufferIsEmpty(MaaImageListBuffer* handle);
    MAA_FRAMEWORK_API MaaBool MaaImageListBufferClear(MaaImageListBuffer* handle);
    MAA_FRAMEWORK_API MaaSize MaaImageListBufferSize(MaaImageListBuffer* handle);
    // return an image view, don't destory it
    MAA_FRAMEWORK_API MaaImageBuffer* MaaImageListBufferAt(MaaImageListBuffer* handle, MaaSize index);
    // append a deep copy of value, you still need to destory it.
    MAA_FRAMEWORK_API MaaBool MaaImageListBufferAppend(MaaImageListBuffer* handle, MaaImageBuffer* value);
    MAA_FRAMEWORK_API MaaBool MaaImageListBufferRemove(MaaImageListBuffer* handle, MaaSize index);

    MAA_FRAMEWORK_API MaaRect* MaaRectBufferCreate();
    MAA_FRAMEWORK_API void MaaRectBufferDestroy(MaaRect* handle);
    MAA_FRAMEWORK_API int32_t MaaRectGetX(MaaRect* handle);
    MAA_FRAMEWORK_API int32_t MaaRectGetY(MaaRect* handle);
    MAA_FRAMEWORK_API int32_t MaaRectGetW(MaaRect* handle);
    MAA_FRAMEWORK_API int32_t MaaRectGetH(MaaRect* handle);

    MAA_FRAMEWORK_API MaaBool MaaRectSet(MaaRect* handle, int32_t x, int32_t y, int32_t w, int32_t h);

#ifdef __cplusplus
}
#endif
