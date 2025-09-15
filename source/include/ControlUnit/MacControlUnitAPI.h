#pragma once

#include <memory>
#include <optional>

#include "ControlUnit/ControlUnitAPI.h"
#include "MaaFramework/MaaDef.h"

#ifdef __cplusplus
extern "C"
{
#endif

    MAA_CONTROL_UNIT_API const char* MaaMacControlUnitGetVersion();

    MAA_CONTROL_UNIT_API MaaControlUnitHandle
        MaaMacControlUnitCreate(uint32_t windowId, MaaMacScreencapMethod screencap_method, MaaMacInputMethod input_method);

    MAA_CONTROL_UNIT_API void MaaMacControlUnitDestroy(MaaControlUnitHandle handle);

#ifdef __cplusplus
}
#endif
