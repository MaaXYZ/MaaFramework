#pragma once

#include "MaaControlUnit/ControlUnitAPI.h"
#include "MaaFramework/MaaDef.h"

#ifdef __cplusplus
extern "C"
{
#endif

    MAA_CONTROL_UNIT_API const char* MaaGamescopeControlUnitGetVersion();

    MAA_CONTROL_UNIT_API MaaGamescopeControlUnitHandle
        MaaGamescopeControlUnitCreate(uint32_t node_id, const char* eis_socket_path, MaaBool use_win32_vk_code);

    MAA_CONTROL_UNIT_API void MaaGamescopeControlUnitDestroy(MaaGamescopeControlUnitHandle handle);

#ifdef __cplusplus
}
#endif
