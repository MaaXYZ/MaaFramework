#pragma once

#include <memory>
#include <optional>

#include "ControlUnitAPI.h"
#include "MaaFramework/MaaDef.h"

#ifdef __cplusplus
extern "C"
{
#endif

    MAA_CONTROL_UNIT_API const char* MaaDbgControlUnitGetVersion();

    MAA_CONTROL_UNIT_API MaaControlUnitHandle MaaDbgControlUnitCreate(MaaDbgControllerType type, const char* read_path);

    MAA_CONTROL_UNIT_API void MaaDbgControlUnitDestroy(MaaControlUnitHandle handle);

#ifdef __cplusplus
}
#endif
