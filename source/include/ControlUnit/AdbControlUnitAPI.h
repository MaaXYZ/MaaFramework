#pragma once

#include <memory>
#include <optional>

#include "ControlUnit/ControlUnitAPI.h"
#include "MaaFramework/MaaDef.h"

#ifdef __cplusplus
extern "C"
{
#endif

    MAA_CONTROL_UNIT_API MaaStringView MaaAdbControlUnitGetVersion();

    MAA_CONTROL_UNIT_API MaaControlUnitHandle MaaAdbControlUnitCreate(
        MaaStringView adb_path,
        MaaStringView adb_serial,
        MaaAdbControllerType type,
        MaaStringView config,
        MaaStringView agent_path,
        MaaControllerCallback callback,
        MaaCallbackTransparentArg callback_arg);

    MAA_CONTROL_UNIT_API void MaaAdbControlUnitDestroy(MaaControlUnitHandle handle);

#ifdef __cplusplus
}
#endif
