#pragma once

#include "ControlUnitAPI.h"
#include "MaaFramework/MaaDef.h"

#ifdef __cplusplus
extern "C"
{
#endif

    MaaStringView MAA_CONTROL_UNIT_API MaaThriftControlUnitGetVersion();

    MaaControlUnitHandle MAA_CONTROL_UNIT_API MaaThriftControlUnitCreate( //
        MaaThriftControllerType type, MaaStringView host, int32_t port, MaaStringView config);

    void MAA_CONTROL_UNIT_API MaaThriftControlUnitDestroy(MaaControlUnitHandle handle);

#ifdef __cplusplus
}
#endif
