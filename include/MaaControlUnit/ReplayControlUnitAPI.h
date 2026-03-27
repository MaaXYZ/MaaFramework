#pragma once

#include "MaaControlUnit/ControlUnitAPI.h"

#ifdef __cplusplus
extern "C"
{
#endif

    MAA_CONTROL_UNIT_API const char* MaaReplayControlUnitGetVersion();

    MAA_CONTROL_UNIT_API MaaReplayControlUnitHandle MaaReplayControlUnitCreate(const char* recording_path);

    MAA_CONTROL_UNIT_API void MaaReplayControlUnitDestroy(MaaReplayControlUnitHandle handle);

#ifdef __cplusplus
}
#endif
