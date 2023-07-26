#pragma once

#include "MaaFramework/MaaDef.h"
#include "MaaFramework/MaaPort.h"

#include "MaaFramework/MaaCustomRecognizer.h"

#ifdef __cplusplus
extern "C"
{
#endif

    struct MAA_FRAMEWORK_API MaaCustomActionAPI
    {
        MaaBool (*run)(MaaJsonString custom_action_param, const MaaRect* cur_box,
                       MaaJsonString recognition_result_detail);

        void (*stop)(void);
    };

#ifdef __cplusplus
}
#endif
