#pragma once

#include <memory>
#include <optional>

#include "ControlUnit/ControlUnitAPI.h"
#include "MaaFramework/MaaDef.h"

#ifdef __cplusplus
extern "C"
{
#endif

    MaaStringView MAA_CONTROL_UNIT_API MaaAdbControlUnitGetVersion();

    MaaControlUnitHandle MAA_CONTROL_UNIT_API MaaAdbControlUnitCreate( //
        MaaStringView adb_path, MaaStringView adb_serial, MaaAdbControllerType type, MaaStringView config,
        MaaStringView agent_path, MaaControllerCallback callback, MaaCallbackTransparentArg callback_arg);

    void MAA_CONTROL_UNIT_API MaaAdbControlUnitDestroy(MaaControlUnitHandle handle);

#ifdef __cplusplus
}
#endif
