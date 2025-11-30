#pragma once

#include "ControlUnit/ControlUnitAPI.h"
#include "MaaFramework/MaaDef.h"

#ifdef __cplusplus
extern "C"
{
#endif

    MAA_CONTROL_UNIT_API const char* MaaAndroidNativeControlUnitGetVersion();

    MAA_CONTROL_UNIT_API MaaControlUnitHandle MaaAndroidNativeControlUnitCreate();

    MAA_CONTROL_UNIT_API void MaaAndroidNativeControlUnitDestroy(MaaControlUnitHandle handle);

#ifdef __cplusplus
}
#endif
