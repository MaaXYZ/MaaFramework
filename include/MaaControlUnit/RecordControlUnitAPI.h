#pragma once

#include "MaaControlUnit/ControlUnitAPI.h"
#include "MaaFramework/MaaDef.h"

#ifdef __cplusplus
extern "C"
{
#endif

    MAA_CONTROL_UNIT_API const char* MaaRecordControlUnitGetVersion();

    /// @param shared_inner Pointer to std::shared_ptr<ControlUnitAPI> (passed as void* across DLL boundary).
    MAA_CONTROL_UNIT_API MaaRecordControlUnitHandle MaaRecordControlUnitCreate(void* shared_inner, const char* recording_path);

    MAA_CONTROL_UNIT_API void MaaRecordControlUnitDestroy(MaaRecordControlUnitHandle handle);

#ifdef __cplusplus
}
#endif
