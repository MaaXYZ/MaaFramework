#pragma once

#include "MaaDef.h"
#include "MaaPort.h"

#include "MaaCustomRecognizer.h"

#ifdef __cplusplus
extern "C"
{
#endif

    struct MAA_FRAMEWORK_API MaaCustomActionAPI
    {
        MaaBool (*run)(MaaSyncContextHandle sync_context, MaaString task_name, MaaJsonString custom_action_param,
                       const MaaRect* cur_box);

        void (*stop)(void);
    };

#ifdef __cplusplus
}
#endif
