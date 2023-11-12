#pragma once

#include <memory>
#include <optional>

#include "ControlUnit/ControlUnitAPI.h"
#include "MaaFramework/MaaDef.h"

#ifdef __cplusplus
extern "C"
{
#endif

    MaaStringView MAA_CONTROL_UNIT_API get_version();

    MaaControlUnitHandle MAA_CONTROL_UNIT_API create_control_unit( //
        void* hWnd, MaaWin32ControllerType type, MaaControllerCallback callback,
        MaaCallbackTransparentArg callback_arg);

    void MAA_CONTROL_UNIT_API destroy_control_unit(MaaControlUnitHandle handle);

#ifdef __cplusplus
}
#endif
