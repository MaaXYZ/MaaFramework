#pragma once

#include "ControlUnitAPI.h"
#include "MaaFramework/MaaDef.h"

#ifdef __cplusplus
extern "C"
{
#endif

    MaaStringView MAA_CONTROL_UNIT_API get_version();

    MaaControlUnitHandle MAA_CONTROL_UNIT_API create_control_unit( //
        MaaThriftControllerType type, MaaStringView host, int32_t port, MaaStringView config);

    void MAA_CONTROL_UNIT_API destroy_control_unit(MaaControlUnitHandle handle);

#ifdef __cplusplus
}
#endif
