#pragma once

#include "MaaDef.h"
#include "MaaPort.h"

#ifdef __cplusplus
extern "C"
{
#endif

    struct MAA_FRAMEWORK_API MaaCustomRecognizerAPI
    {
        MaaBool (*analyze)(MaaSyncContextHandle sync_context, const MaaImageBufferHandle image, MaaStringView task_name,
                           MaaStringView custom_recognition_param,
                           /*out*/ MaaRectHandle box,
                           /*out*/ MaaStringBufferHandle detail_buff);
    };

#ifdef __cplusplus
}
#endif
