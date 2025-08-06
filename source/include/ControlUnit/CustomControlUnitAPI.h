#pragma once

#include <memory>
#include <optional>

#include "ControlUnit/ControlUnitAPI.h"
#include "MaaFramework/Instance/MaaCustomController.h"
#include "MaaFramework/MaaDef.h"

#ifdef __cplusplus
extern "C"
{
#endif

    MAA_CONTROL_UNIT_API const char* MaaCustomControlUnitGetVersion();

    MAA_CONTROL_UNIT_API MaaCustomControlUnitHandle
        MaaCustomControlUnitCreate(MaaCustomControllerCallbacks* controller, void* controller_arg);

    MAA_CONTROL_UNIT_API void MaaCustomControlUnitDestroy(MaaCustomControlUnitHandle handle);

#ifdef __cplusplus
}
#endif
