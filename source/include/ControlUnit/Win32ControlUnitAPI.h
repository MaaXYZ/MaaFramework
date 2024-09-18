#pragma once

#include <memory>
#include <optional>

#include "ControlUnit/ControlUnitAPI.h"
#include "MaaFramework/MaaDef.h"

#ifdef __cplusplus
extern "C"
{
#endif

    MAA_CONTROL_UNIT_API const char* MaaWin32ControlUnitGetVersion();

    MAA_CONTROL_UNIT_API MaaControlUnitHandle
        MaaWin32ControlUnitCreate(void* hWnd, MaaWin32ScreencapMethod screencap_method, MaaWin32InputMethod input_method);

    MAA_CONTROL_UNIT_API void MaaWin32ControlUnitDestroy(MaaControlUnitHandle handle);

#ifdef __cplusplus
}
#endif
