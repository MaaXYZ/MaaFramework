#pragma once

#include "../MaaToolkitDef.h"

#ifdef __cplusplus
extern "C"
{
#endif

    // May run on a worker thread; must be thread-safe and must not throw or reenter.
    // Returning true cancels discovery. Borrowed until FindWithOptions returns.
    typedef MaaBool(MAA_CALL* MaaToolkitIOSDeviceCancelCallback)(void* userdata);

    MAA_TOOLKIT_API MaaToolkitIOSDeviceList* MaaToolkitIOSDeviceListCreate();
    MAA_TOOLKIT_API void MaaToolkitIOSDeviceListDestroy(MaaToolkitIOSDeviceList* list);
    // USB and Wi-Fi snapshot; no pairing, DDI installation or media startup.
    MAA_TOOLKIT_API MaaBool MaaToolkitIOSDeviceFind(MaaToolkitIOSDeviceList* list);
    // Options: timeout_ms (0..60000, default 3000), server_port (XDB_PORT or 17423).
    // USB enumeration has a separate one-second budget. cancel_callback and userdata may be null.
    // Returns false on failure/cancellation; inspect ListGetWarnings for diagnostics.
    MAA_TOOLKIT_API MaaBool MaaToolkitIOSDeviceFindWithOptions(
        MaaIOSConnectionMethod connection_method,
        const char* options_json,
        MaaToolkitIOSDeviceCancelCallback cancel_callback,
        void* userdata,
        MaaToolkitIOSDeviceList* list);
    MAA_TOOLKIT_API MaaSize MaaToolkitIOSDeviceListSize(const MaaToolkitIOSDeviceList* list);
    // Borrowed results remain valid until the next find or list destruction.
    MAA_TOOLKIT_API const MaaToolkitIOSDevice* MaaToolkitIOSDeviceListAt(const MaaToolkitIOSDeviceList* list, MaaSize index);
    MAA_TOOLKIT_API const char* MaaToolkitIOSDeviceGetName(const MaaToolkitIOSDevice* device);
    MAA_TOOLKIT_API const char* MaaToolkitIOSDeviceGetUdid(const MaaToolkitIOSDevice* device);
    // discovered / paired / connected. A record does not prove connectivity;
    // connected only means xdb has an active connection/session.
    MAA_TOOLKIT_API const char* MaaToolkitIOSDeviceGetState(const MaaToolkitIOSDevice* device);
    // JSON array retains transport, address, port, interface and pairing state.
    MAA_TOOLKIT_API const char* MaaToolkitIOSDeviceGetConnections(const MaaToolkitIOSDevice* device);
    MAA_TOOLKIT_API MaaIOSScreencapMethod MaaToolkitIOSDeviceGetScreencapMethods(const MaaToolkitIOSDevice* device);
    MAA_TOOLKIT_API const char* MaaToolkitIOSDeviceListGetWarnings(const MaaToolkitIOSDeviceList* list);

#ifdef __cplusplus
}
#endif
