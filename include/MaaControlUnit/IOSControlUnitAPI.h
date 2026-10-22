#pragma once

#include "MaaControlUnit/ControlUnitAPI.h"
#include "MaaFramework/MaaDef.h"

#ifdef __cplusplus
extern "C"
{
#endif

    MAA_CONTROL_UNIT_API const char* MaaIOSControlUnitGetVersion();

    /**
     * Create an iOS controller using a running XDeviceBridge server.
     * udid is required; transport and capture selections use their iOS enums.
     * config_json: optional server_port/server_token; omitted values use
     * XDB_PORT (or 17423) / XDB_TOKEN. xdb owns phone credentials and DDI setup.
     * Stream obtains decoded BGRA; ScreenshotService obtains lossless PNG.
     * Single-touch coordinates use the last delivered image geometry.
     * Text accepts US-layout ASCII, tab and newline; unsupported text is rejected.
     */
    MAA_CONTROL_UNIT_API MaaIOSControlUnitHandle MaaIOSControlUnitCreate(
        const char* udid,
        MaaIOSScreencapMethod screencap_methods,
        MaaIOSConnectionMethod connection_method,
        const char* wifi_address,
        const char* config_json);

    MAA_CONTROL_UNIT_API void MaaIOSControlUnitDestroy(MaaIOSControlUnitHandle handle);

#ifdef __cplusplus
}
#endif
