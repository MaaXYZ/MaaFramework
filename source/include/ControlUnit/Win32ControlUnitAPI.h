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

    MAA_CONTROL_UNIT_API MaaWin32ControlUnitHandle MaaWin32ControlUnitCreate(
        void* hWnd,
        MaaWin32ScreencapMethod screencap_method,
        MaaWin32InputMethod mouse_method,
        MaaWin32InputMethod keyboard_method);

    MAA_CONTROL_UNIT_API void MaaWin32ControlUnitDestroy(MaaWin32ControlUnitHandle handle);

#ifdef __cplusplus
}
#endif
