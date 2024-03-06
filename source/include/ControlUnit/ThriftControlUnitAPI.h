#pragma once

#include "ControlUnitAPI.h"
#include "MaaFramework/MaaDef.h"

#ifdef __cplusplus
extern "C"
{
#endif

    MAA_CONTROL_UNIT_API MaaStringView MaaThriftControlUnitGetVersion();

    MAA_CONTROL_UNIT_API MaaControlUnitHandle MaaThriftControlUnitCreate(
        MaaThriftControllerType type,
        MaaStringView host,
        int32_t port,
        MaaStringView config);

    MAA_CONTROL_UNIT_API void MaaThriftControlUnitDestroy(MaaControlUnitHandle handle);

#ifdef __cplusplus
}
#endif
