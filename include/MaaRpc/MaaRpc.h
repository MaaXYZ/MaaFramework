#pragma once

#include "MaaFramework/MaaDef.h"

#ifdef __cplusplus
extern "C"
{
#endif

    MaaBool MAA_RPC_API MaaRpcStart(MaaStringView address);
    void MAA_RPC_API MaaRpcStop();
    void MAA_RPC_API MaaRpcWait();

#ifdef __cplusplus
}
#endif
