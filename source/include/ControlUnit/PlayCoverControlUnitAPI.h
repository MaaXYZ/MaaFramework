#pragma once

#include "ControlUnit/ControlUnitAPI.h"
#include "MaaFramework/MaaDef.h"

#ifdef __cplusplus
extern "C"
{
#endif

    MAA_CONTROL_UNIT_API const char* MaaPlayCoverControlUnitGetVersion();

    MAA_CONTROL_UNIT_API MaaControlUnitHandle MaaPlayCoverControlUnitCreate(const char* address, const char* uuid);

    MAA_CONTROL_UNIT_API void MaaPlayCoverControlUnitDestroy(MaaControlUnitHandle handle);

#ifdef __cplusplus
}
#endif
