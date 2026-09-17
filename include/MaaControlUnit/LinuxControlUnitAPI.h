#pragma once

#include "MaaControlUnit/ControlUnitAPI.h"
#include "MaaFramework/MaaDef.h"

#ifdef __cplusplus
extern "C"
{
#endif

    MAA_CONTROL_UNIT_API const char* MaaLinuxControlUnitGetVersion();

    MAA_CONTROL_UNIT_API MaaLinuxControlUnitHandle MaaLinuxControlUnitCreate(const char* config_json);

    MAA_CONTROL_UNIT_API void MaaLinuxControlUnitDestroy(MaaLinuxControlUnitHandle handle);

#ifdef __cplusplus
}
#endif
