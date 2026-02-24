#pragma once

#include "ControlUnitAPI.h"
#include "MaaFramework/MaaDef.h"

#ifdef __cplusplus
extern "C"
{
#endif

    MAA_CONTROL_UNIT_API const char* MaaProxyControlUnitGetVersion();

    MAA_CONTROL_UNIT_API MaaControlUnitHandle MaaProxyControlUnitCreate(void* shared_inner, const char* dump_dir);

    MAA_CONTROL_UNIT_API void MaaProxyControlUnitDestroy(MaaControlUnitHandle handle);

#ifdef __cplusplus
}
#endif
