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
    MAA_FRAMEWORK_API MaaBool MaaStringBufferIsEmpty(const MaaStringBuffer* handle);
    MAA_FRAMEWORK_API MaaBool MaaStringBufferClear(MaaStringBuffer* handle);

    MAA_FRAMEWORK_API const char* MaaStringBufferGet(const MaaStringBuffer* handle);
    MAA_FRAMEWORK_API MaaSize MaaStringBufferSize(const MaaStringBuffer* handle);
    MAA_FRAMEWORK_API MaaBool MaaStringBufferSet(MaaStringBuffer* handle, const char* str);
    MAA_FRAMEWORK_API MaaBool MaaStringBufferSetEx(MaaStringBuffer* handle, const char* str, MaaSize size);

    MAA_FRAMEWORK_API MaaStringListBuffer* MaaStringListBufferCreate();
    MAA_FRAMEWORK_API void MaaStringListBufferDestroy(MaaStringListBuffer* handle);
    MAA_FRAMEWORK_API MaaBool MaaStringListBufferIsEmpty(const MaaStringListBuffer* handle);
    MAA_FRAMEWORK_API MaaSize MaaStringListBufferSize(const MaaStringListBuffer* handle);
    // return a string view, don't destory it
    MAA_FRAMEWORK_API const MaaStringBuffer* MaaStringListBufferAt(const MaaStringListBuffer* handle, MaaSize index);

    // append a deep copy of value, you still need to destory it.
    MAA_FRAMEWORK_API MaaBool MaaStringListBufferAppend(MaaStringListBuffer* handle, const MaaStringBuffer* value);
    MAA_FRAMEWORK_API MaaBool MaaStringListBufferRemove(MaaStringListBuffer* handle, MaaSize index);
    MAA_FRAMEWORK_API MaaBool MaaStringListBufferClear(MaaStringListBuffer* handle);

    MAA_FRAMEWORK_API MaaImageBuffer* MaaImageBufferCreate();
    MAA_FRAMEWORK_API void MaaImageBufferDestroy(MaaImageBuffer* handle);
    MAA_FRAMEWORK_API MaaBool MaaImageBufferIsEmpty(const MaaImageBuffer* handle);
    MAA_FRAMEWORK_API MaaBool MaaImageBufferClear(MaaImageBuffer* handle);

    typedef void* MaaImageRawData;
    MAA_FRAMEWORK_API MaaImageRawData MaaImageBufferGetRawData(const MaaImageBuffer* handle);
    MAA_FRAMEWORK_API int32_t MaaImageBufferWidth(const MaaImageBuffer* handle);
    MAA_FRAMEWORK_API int32_t MaaImageBufferHeight(const MaaImageBuffer* handle);
    MAA_FRAMEWORK_API int32_t MaaImageBufferChannels(const MaaImageBuffer* handle);
    MAA_FRAMEWORK_API int32_t MaaImageBufferType(const MaaImageBuffer* handle);
    MAA_FRAMEWORK_API MaaBool
        MaaImageBufferSetRawData(MaaImageBuffer* handle, MaaImageRawData data, int32_t width, int32_t height, int32_t type);
    MAA_FRAMEWORK_API MaaBool MaaImageBufferResize(MaaImageBuffer* handle, int32_t width, int32_t height);

    typedef uint8_t* MaaImageEncodedData;
    MAA_FRAMEWORK_API MaaImageEncodedData MaaImageBufferGetEncoded(const MaaImageBuffer* handle);
    MAA_FRAMEWORK_API MaaSize MaaImageBufferGetEncodedSize(const MaaImageBuffer* handle);
    MAA_FRAMEWORK_API MaaBool MaaImageBufferSetEncoded(MaaImageBuffer* handle, MaaImageEncodedData data, MaaSize size);

    MAA_FRAMEWORK_API MaaImageListBuffer* MaaImageListBufferCreate();
    MAA_FRAMEWORK_API void MaaImageListBufferDestroy(MaaImageListBuffer* handle);
    MAA_FRAMEWORK_API MaaBool MaaImageListBufferIsEmpty(const MaaImageListBuffer* handle);
    MAA_FRAMEWORK_API MaaSize MaaImageListBufferSize(const MaaImageListBuffer* handle);
    // return an image view, don't destory it
    MAA_FRAMEWORK_API const MaaImageBuffer* MaaImageListBufferAt(const MaaImageListBuffer* handle, MaaSize index);

    // append a deep copy of value, you still need to destory it.
    MAA_FRAMEWORK_API MaaBool MaaImageListBufferAppend(MaaImageListBuffer* handle, const MaaImageBuffer* value);
    MAA_FRAMEWORK_API MaaBool MaaImageListBufferRemove(MaaImageListBuffer* handle, MaaSize index);
    MAA_FRAMEWORK_API MaaBool MaaImageListBufferClear(MaaImageListBuffer* handle);

    MAA_FRAMEWORK_API MaaRect* MaaRectCreate();
    MAA_FRAMEWORK_API void MaaRectDestroy(MaaRect* handle);
    MAA_FRAMEWORK_API int32_t MaaRectGetX(const MaaRect* handle);
    MAA_FRAMEWORK_API int32_t MaaRectGetY(const MaaRect* handle);
    MAA_FRAMEWORK_API int32_t MaaRectGetW(const MaaRect* handle);
    MAA_FRAMEWORK_API int32_t MaaRectGetH(const MaaRect* handle);

    MAA_FRAMEWORK_API MaaBool MaaRectSet(MaaRect* handle, int32_t x, int32_t y, int32_t w, int32_t h);

#ifdef __cplusplus
}
#endif
