#pragma once

#include <memory>
#include <optional>

#include "ControlUnit/ControlUnitAPI.h"
#include "MaaFramework/MaaDef.h"

#ifdef __cplusplus
extern "C"
{
#endif

    MaaStringView MAA_CONTROL_UNIT_API MaaWin32ControlUnitGetVersion();

    MaaControlUnitHandle MAA_CONTROL_UNIT_API MaaWin32ControlUnitCreate( //
        MaaWin32Hwnd hWnd, MaaWin32ControllerType type, MaaControllerCallback callback,
        MaaCallbackTransparentArg callback_arg);

    void MAA_CONTROL_UNIT_API MaaWin32ControlUnitDestroy(MaaControlUnitHandle handle);

#ifdef __cplusplus
}
#endif
