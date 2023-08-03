#pragma once

#include "MaaFramework/MaaDef.h"
#include "MaaFramework/MaaPort.h"

#ifdef __cplusplus
extern "C"
{
#endif

    struct MAA_FRAMEWORK_API MaaRect
    {
        int32_t x;
        int32_t y;
        int32_t width;
        int32_t height;
    };

    struct MAA_FRAMEWORK_API MaaImage
    {
        int32_t rows;
        int32_t cols;
        int32_t type;
        void* data;
    };

#define MaaRecognitionResultDetailBuffSize 16384

    struct MAA_FRAMEWORK_API MaaRecognitionResult
    {
        MaaRect box;
        void* detail_buff; // size = MaaRecognitionResultDetailBuffSize
    };

    struct MAA_FRAMEWORK_API MaaCustomRecognizerAPI
    {
        MaaBool (*analyze)(MaaSyncContextHandle sync_context, const MaaImage* image, MaaString task_name,
                           MaaJsonString custom_recognition_param,
                           /*out*/ MaaRecognitionResult* result);
    };

#ifdef __cplusplus
}
#endif
