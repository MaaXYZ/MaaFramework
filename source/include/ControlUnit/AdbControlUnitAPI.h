#pragma once

#include <memory>
#include <optional>

#include "ControlUnit/ControlUnitAPI.h"
#include "MaaFramework/MaaDef.h"

#ifdef __cplusplus
extern "C"
{
#endif

    MAA_CONTROL_UNIT_API const char* MaaAdbControlUnitGetVersion();

    MAA_CONTROL_UNIT_API MaaAdbControlUnitHandle MaaAdbControlUnitCreate(
        const char* adb_path,
        const char* adb_serial,
        MaaAdbScreencapMethod screencap_methods,
        MaaAdbInputMethod input_methods,
        const char* config,
        const char* agent_path);

    MAA_CONTROL_UNIT_API void MaaAdbControlUnitDestroy(MaaAdbControlUnitHandle handle);

#ifdef __cplusplus
}
#endif
