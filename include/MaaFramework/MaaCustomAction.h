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
        MaaBool (*run)(MaaSyncContextHandle sync_context, MaaString task_name, MaaString custom_action_param,
                       MaaRectHandle cur_box, MaaString cur_rec_detail);

        void (*stop)(void);
    };

#ifdef __cplusplus
}
#endif
