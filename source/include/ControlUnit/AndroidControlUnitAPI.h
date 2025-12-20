#pragma once

#include <memory>
#include <optional>

#include "ControlUnit/ControlUnitAPI.h"
#include "MaaFramework/MaaDef.h"

#ifdef __cplusplus
extern "C"
{
#endif

    MAA_CONTROL_UNIT_API const char* MaaAndroidControlUnitGetVersion();

    /**
     * @brief Create Android control unit based on accessibility / media projection.
     *
     * @param screencap_methods bitmask of MaaAndroidScreencapMethod
     * @param input_methods bitmask of MaaAndroidInputMethod
     * @return MaaAndroidControlUnitHandle
     */
    MAA_CONTROL_UNIT_API MaaAndroidControlUnitHandle
        MaaAndroidControlUnitCreate(MaaAndroidScreencapMethod screencap_methods, MaaAndroidInputMethod input_methods);

    MAA_CONTROL_UNIT_API void MaaAndroidControlUnitDestroy(MaaAndroidControlUnitHandle handle);

#ifdef __cplusplus
}
#endif
