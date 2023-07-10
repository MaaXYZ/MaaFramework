#pragma once

#include "MaaDef.h"
#include "MaaPort.h"

#ifdef __cplusplus
extern "C"
{
#endif
    struct MAA_API MaaCustomTaskAPI
    {
        MaaBool (*run)(void);
        MaaBool (*set_param)(MaaJsonString param);
    };

#ifdef __cplusplus
}
#endif
