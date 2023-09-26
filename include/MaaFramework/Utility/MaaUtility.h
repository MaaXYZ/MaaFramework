#pragma once

#include "../MaaDef.h"
#include "../MaaPort.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /* Global */
    MaaStringView MAA_FRAMEWORK_API MaaVersion();
    MaaBool MAA_FRAMEWORK_API MaaSetGlobalOption(MaaGlobalOption key, MaaOptionValue value,
                                                 MaaOptionValueSize val_size);

#ifdef __cplusplus
}
#endif
