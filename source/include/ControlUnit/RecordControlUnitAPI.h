#pragma once

#include "ControlUnitAPI.h"
#include "MaaFramework/MaaDef.h"

#ifdef __cplusplus
extern "C"
{
#endif

    MAA_CONTROL_UNIT_API const char* MaaRecordControlUnitGetVersion();

    MAA_CONTROL_UNIT_API MaaRecordControlUnitHandle MaaRecordControlUnitCreate(void* shared_inner, const char* dump_dir);

    MAA_CONTROL_UNIT_API void MaaRecordControlUnitDestroy(MaaRecordControlUnitHandle handle);

#ifdef __cplusplus
}
#endif
