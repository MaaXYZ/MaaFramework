#pragma once

#include <memory>
#include <optional>

#include "ControlUnit/ControlUnitAPI.h"
#include "MaaFramework/MaaDef.h"

#ifdef __cplusplus
extern "C"
{
#endif

    MAA_CONTROL_UNIT_API const char* MaaMacOSControlUnitGetVersion();

    MAA_CONTROL_UNIT_API MaaMacOSControlUnitHandle MaaMacOSControlUnitCreate(
        uint32_t window_id,
        pid_t pid,
        MaaMacOSScreencapMethod screencap_method,
        MaaMacOSInputMethod input_method);

    MAA_CONTROL_UNIT_API void MaaMacOSControlUnitDestroy(MaaMacOSControlUnitHandle handle);

#ifdef __cplusplus
}
#endif
