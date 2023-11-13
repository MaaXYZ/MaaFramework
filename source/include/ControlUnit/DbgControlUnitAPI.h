#pragma once

#include <memory>
#include <optional>

#include "ControlUnitAPI.h"
#include "MaaFramework/MaaDef.h"

#ifdef __cplusplus
extern "C"
{
#endif

    MaaStringView MAA_CONTROL_UNIT_API MaaDbgControlUnitGetVersion();

    MaaControlUnitHandle MAA_CONTROL_UNIT_API MaaDbgControlUnitCreate( //
        MaaDbgControllerType type, MaaStringView read_path);

    void MAA_CONTROL_UNIT_API MaaDbgControlUnitDestroy(MaaControlUnitHandle handle);

#ifdef __cplusplus
}
#endif
