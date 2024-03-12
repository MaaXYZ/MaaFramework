#pragma once

#include <memory>
#include <optional>

#include "ControlUnit/ControlUnitAPI.h"
#include "MaaFramework/MaaDef.h"

#ifdef __cplusplus
extern "C"
{
#endif

    MAA_CONTROL_UNIT_API MaaStringView MaaWin32ControlUnitGetVersion();

    MAA_CONTROL_UNIT_API MaaControlUnitHandle MaaWin32ControlUnitCreate(
        MaaWin32Hwnd hWnd,
        MaaWin32ControllerType type,
        MaaControllerCallback callback,
        MaaCallbackTransparentArg callback_arg);

    MAA_CONTROL_UNIT_API void MaaWin32ControlUnitDestroy(MaaControlUnitHandle handle);

#ifdef __cplusplus
}
#endif
