#pragma once

#include "MaaControlUnit/ControlUnitAPI.h"
#include "MaaFramework/MaaDef.h"

#ifdef __cplusplus
extern "C"
{
#endif

    MAA_CONTROL_UNIT_API const char* MaaAndroidNativeControlUnitGetVersion();

    MAA_CONTROL_UNIT_API MaaAndroidNativeControlUnitHandle MaaAndroidNativeControlUnitCreate(const char* config_json);

    MAA_CONTROL_UNIT_API void MaaAndroidNativeControlUnitDestroy(MaaAndroidNativeControlUnitHandle handle);

#ifdef __cplusplus
}
#endif
