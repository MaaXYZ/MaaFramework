#pragma once

#include "../MaaDef.h"
#include "../MaaPort.h"

#ifdef __cplusplus
extern "C"
{
#endif

    struct MaaCustomActionAPI
    {
        MaaBool (*run)(MaaSyncContextHandle sync_context, MaaStringView task_name, MaaStringView custom_action_param,
                       MaaRectHandle cur_box, MaaStringView cur_rec_detail, MaaTransparentArg action_arg);

        void (*stop)(MaaTransparentArg action_arg);
    };

#ifdef __cplusplus
}
#endif
