#pragma once

#include "MaaDef.h"
#include "MaaPort.h"

#include "MaaCustomRecognizer.h"

#ifdef __cplusplus
extern "C"
{
#endif

    struct MaaCustomActionAPI
    {
        MaaBool (*run)(MaaSyncContextHandle sync_context, MaaStringView task_name, MaaStringView custom_action_param,
                       MaaRectHandle cur_box, MaaStringView cur_rec_detail);

        void (*stop)(void);
    };

#ifdef __cplusplus
}
#endif
