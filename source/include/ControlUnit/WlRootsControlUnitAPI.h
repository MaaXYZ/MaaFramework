#pragma once

#include "ControlUnit/ControlUnitAPI.h"
#include "MaaFramework/MaaDef.h"

#ifdef __cplusplus
extern "C"
{
#endif

    MAA_CONTROL_UNIT_API const char* MaaWlRootsControlUnitGetVersion();

    MAA_CONTROL_UNIT_API MaaControlUnitHandle MaaWlRootsControlUnitCreate(const char* wlr_socket_path);

    MAA_CONTROL_UNIT_API void MaaWlRootsControlUnitDestroy(MaaControlUnitHandle handle);

#ifdef __cplusplus
}
#endif
