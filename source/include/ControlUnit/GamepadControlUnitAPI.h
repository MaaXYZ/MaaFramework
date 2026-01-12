#pragma once

#include "ControlUnit/ControlUnitAPI.h"
#include "MaaFramework/MaaDef.h"

#ifdef __cplusplus
extern "C"
{
#endif

    MAA_CONTROL_UNIT_API const char* MaaGamepadControlUnitGetVersion();

    MAA_CONTROL_UNIT_API MaaControlUnitHandle MaaGamepadControlUnitCreate(void* hWnd, MaaWin32ScreencapMethod screencap_method);

    MAA_CONTROL_UNIT_API void MaaGamepadControlUnitDestroy(MaaControlUnitHandle handle);

#ifdef __cplusplus
}
#endif
