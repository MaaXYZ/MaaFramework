#pragma once

#include <memory>
#include <optional>

#include "ControlUnitAPI.h"
#include "MaaFramework/MaaDef.h"

#ifdef __cplusplus
extern "C"
{
#endif

    MaaStringView MAA_CONTROL_UNIT_API get_version();

    MaaControlUnitHandle MAA_CONTROL_UNIT_API create_control_unit( //
        MaaDbgControllerType type, MaaStringView read_path);

    void MAA_CONTROL_UNIT_API destroy_control_unit(MaaControlUnitHandle handle);

#ifdef __cplusplus
}
#endif
