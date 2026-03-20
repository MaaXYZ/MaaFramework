#pragma once

#include "ControlUnitAPI.h"

#ifdef __cplusplus
extern "C"
{
#endif

    MAA_CONTROL_UNIT_API const char* MaaReplayControlUnitGetVersion();

    MAA_CONTROL_UNIT_API MaaReplayControlUnitHandle MaaReplayControlUnitCreate(const char* dump_dir);

    MAA_CONTROL_UNIT_API void MaaReplayControlUnitDestroy(MaaReplayControlUnitHandle handle);

#ifdef __cplusplus
}
#endif
