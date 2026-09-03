#pragma once

#include "MaaControlUnit/ControlUnitAPI.h"
#include "MaaFramework/MaaDef.h"

#ifdef __cplusplus
extern "C"
{
#endif

    MAA_CONTROL_UNIT_API const char* MaaKWinControlUnitGetVersion();

    MAA_CONTROL_UNIT_API MaaKWinControlUnitHandle
        MaaKWinControlUnitCreate(const char* device_node, int screen_width, int screen_height, MaaBool use_win32_vk_code);

    MAA_CONTROL_UNIT_API void MaaKWinControlUnitDestroy(MaaKWinControlUnitHandle handle);

#ifdef __cplusplus
}
#endif
