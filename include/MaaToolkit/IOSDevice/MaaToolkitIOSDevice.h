#pragma once

#include "../MaaToolkitDef.h"

#ifdef __cplusplus
extern "C"
{
#endif

    // Synchronous operations. result receives JSON with success,
    // cancelled and error, including on failure.
    // event_json has type progress or pin. Returning false cancels the operation.
    // Event callbacks are serialized; cancellation callbacks may run on worker threads
    // and must be thread-safe. Callbacks must not throw or reenter the operation.
    // Callbacks and userdata are borrowed until return; no operation jobs remain afterward.
    typedef MaaBool(MAA_CALL* MaaToolkitIOSDeviceEventCallback)(const char* event_json, void* userdata);
    typedef MaaBool(MAA_CALL* MaaToolkitIOSDeviceCancelCallback)(void* userdata);

    // Discover through xdb Server. Options: timeout_ms, server_port, server_token.
    // Empty server settings use XDB_PORT / XDB_TOKEN.
    MAA_TOOLKIT_API MaaBool MaaToolkitIOSDeviceDiscover(
        MaaIOSConnectionMethod connection_method,
        const char* options_json,
        MaaToolkitIOSDeviceEventCallback event_callback,
        MaaToolkitIOSDeviceCancelCallback cancel_callback,
        void* userdata,
        MaaStringBuffer* result);

    MAA_TOOLKIT_API MaaToolkitIOSDeviceList* MaaToolkitIOSDeviceListCreate();
    MAA_TOOLKIT_API void MaaToolkitIOSDeviceListDestroy(MaaToolkitIOSDeviceList* list);
    // USB and Wi-Fi snapshot; no pairing, DDI installation or media startup.
    MAA_TOOLKIT_API MaaBool MaaToolkitIOSDeviceFind(MaaToolkitIOSDeviceList* list);
    // timeout_ms: 0..60000; USB enumeration has a separate one-second budget.
    MAA_TOOLKIT_API MaaBool MaaToolkitIOSDeviceFindWithTimeout(int64_t timeout_ms, MaaToolkitIOSDeviceList* list);
    // Same connection arguments and options as Discover.
    MAA_TOOLKIT_API MaaBool MaaToolkitIOSDeviceFindWithOptions(
        MaaIOSConnectionMethod connection_method,
        const char* options_json,
        MaaToolkitIOSDeviceList* list);
    MAA_TOOLKIT_API MaaSize MaaToolkitIOSDeviceListSize(const MaaToolkitIOSDeviceList* list);
    // Borrowed results remain valid until the next find or list destruction.
    MAA_TOOLKIT_API const MaaToolkitIOSDevice* MaaToolkitIOSDeviceListAt(const MaaToolkitIOSDeviceList* list, MaaSize index);
    MAA_TOOLKIT_API const char* MaaToolkitIOSDeviceGetName(const MaaToolkitIOSDevice* device);
    MAA_TOOLKIT_API const char* MaaToolkitIOSDeviceGetUdid(const MaaToolkitIOSDevice* device);
    // JSON array retains transport, address, port, interface and pairing state.
    MAA_TOOLKIT_API const char* MaaToolkitIOSDeviceGetConnections(const MaaToolkitIOSDevice* device);
    MAA_TOOLKIT_API MaaIOSScreencapMethod MaaToolkitIOSDeviceGetScreencapMethods(const MaaToolkitIOSDevice* device);
    MAA_TOOLKIT_API const char* MaaToolkitIOSDeviceListGetWarnings(const MaaToolkitIOSDeviceList* list);

#ifdef __cplusplus
}
#endif
