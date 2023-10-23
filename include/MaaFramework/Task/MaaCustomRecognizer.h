#pragma once

#include "../MaaDef.h"
#include "../MaaPort.h"

#ifdef __cplusplus
extern "C"
{
#endif

    struct MaaCustomRecognizerAPI
    {
        MaaBool (*analyze)(MaaSyncContextHandle sync_context, const MaaImageBufferHandle image, MaaStringView task_name,
                           MaaStringView custom_recognition_param, MaaTransparentArg recognizer_arg,
                           /*out*/ MaaRectHandle out_box,
                           /*out*/ MaaStringBufferHandle out_detail);
    };

#ifdef __cplusplus
}
#endif
