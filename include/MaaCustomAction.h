#pragma once

#include "MaaDef.h"
#include "MaaPort.h"

#include "MaaCustomRecognizer.h"

#ifdef __cplusplus
extern "C"
{
#endif

    struct MAA_API MaaCustomActionAPI
    {
        MaaBool (*run)(MaaJsonString custom_action_param, MaaRect cur_box, MaaJsonString recognition_result_detail);

        void (*stop)(void);
    };

#ifdef __cplusplus
}
#endif
