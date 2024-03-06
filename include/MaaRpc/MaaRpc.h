#pragma once

#include "MaaFramework/MaaDef.h"

#ifdef __cplusplus
extern "C"
{
#endif

    MAA_RPC_API MaaBool MaaRpcStart(MaaStringView address);
    MAA_RPC_API void MaaRpcStop();
    MAA_RPC_API void MaaRpcWait();

#ifdef __cplusplus
}
#endif
