#pragma once

#include "MaaControlUnit/ControlUnitAPI.h"

#ifdef __cplusplus
extern "C"
{
#endif

    MAA_CONTROL_UNIT_API const char* MaaDbgControlUnitGetVersion();

    MAA_CONTROL_UNIT_API MaaDbgControlUnitHandle MaaDbgControlUnitCreate(const char* read_path);

    MAA_CONTROL_UNIT_API void MaaDbgControlUnitDestroy(MaaDbgControlUnitHandle handle);

#ifdef __cplusplus
}
#endif
