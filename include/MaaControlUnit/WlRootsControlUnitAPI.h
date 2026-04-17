#pragma once

#include "MaaControlUnit/ControlUnitAPI.h"
#include "MaaFramework/MaaDef.h"

#ifdef __cplusplus
extern "C"
{
#endif

    MAA_CONTROL_UNIT_API const char* MaaWlRootsControlUnitGetVersion();

    MAA_CONTROL_UNIT_API MaaWlRootsControlUnitHandle MaaWlRootsControlUnitCreate(const char* wlr_socket_path, MaaBool use_win32_vk_code);

    MAA_CONTROL_UNIT_API void MaaWlRootsControlUnitDestroy(MaaWlRootsControlUnitHandle handle);

#ifdef __cplusplus
}
#endif
