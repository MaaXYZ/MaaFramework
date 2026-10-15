#pragma once

#include "../MaaToolkitDef.h"

#ifdef __cplusplus
extern "C"
{
#endif

    // Synchronous operations. options_json may include pairing_records: an array
    // of opaque credential strings to copy. result receives JSON with success,
    // cancelled and error, including on failure.
    // event_json has type progress or pin. Returning false cancels the operation.
    // The cancellation callback may run on worker threads and must be thread-safe.
    typedef MaaBool (*MaaToolkitIOSDeviceEventCallback)(const char* event_json, void* userdata);
    typedef MaaBool (*MaaToolkitIOSDeviceCancelCallback)(void* userdata);

    // Options: timeout_ms (0..60000, default 3000), usb_only, wifi_only.
    MAA_TOOLKIT_API MaaBool MaaToolkitIOSDeviceDiscover(
        const char* options_json,
        MaaToolkitIOSDeviceEventCallback event_callback,
        MaaToolkitIOSDeviceCancelCallback cancel_callback,
        void* userdata,
        MaaStringBuffer* result);

    // Options: udid, wifi (empty or "auto"), repair (USB only),
    // timeout_ms (1..300000, default 120000). Wi-Fi requires a PIN event callback.
    // result and credential must be distinct, caller-created MaaStringBuffers.
    // The verified versioned credential is returned separately from status JSON;
    // failures clear credential. Persistence and access control belong to the caller.
    MAA_TOOLKIT_API MaaBool MaaToolkitIOSDevicePair(
        const char* options_json,
        MaaToolkitIOSDeviceEventCallback event_callback,
        MaaToolkitIOSDeviceCancelCallback cancel_callback,
        void* userdata,
        MaaStringBuffer* result,
        MaaStringBuffer* credential);

    // Options: udid, wifi (empty, "auto" or address), port (explicit address only).
    // Closes the verified session before returning; does not create a shared tunnel.
    MAA_TOOLKIT_API MaaBool MaaToolkitIOSDeviceCheckConnection(
        const char* options_json,
        MaaToolkitIOSDeviceEventCallback event_callback,
        MaaToolkitIOSDeviceCancelCallback cancel_callback,
        void* userdata,
        MaaStringBuffer* result);

    // Connection options plus cache_directory, replace (default false),
    // download_timeout_seconds (1..3600, default 300),
    // install_timeout_seconds (1..3600, default 180).
    // Empty cache_directory uses temporary files and cleans them after the operation;
    // only an explicit directory enables persistent caching.
    MAA_TOOLKIT_API MaaBool MaaToolkitIOSDevicePrepare(
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
    // Same options as Discover, including explicit pairing_records.
    MAA_TOOLKIT_API MaaBool MaaToolkitIOSDeviceFindWithOptions(const char* options_json, MaaToolkitIOSDeviceList* list);
    MAA_TOOLKIT_API MaaSize MaaToolkitIOSDeviceListSize(const MaaToolkitIOSDeviceList* list);
    // Borrowed results remain valid until the next find or list destruction.
    MAA_TOOLKIT_API const MaaToolkitIOSDevice* MaaToolkitIOSDeviceListAt(const MaaToolkitIOSDeviceList* list, MaaSize index);
    MAA_TOOLKIT_API const char* MaaToolkitIOSDeviceGetName(const MaaToolkitIOSDevice* device);
    MAA_TOOLKIT_API const char* MaaToolkitIOSDeviceGetUdid(const MaaToolkitIOSDevice* device);
    // JSON array retains transport, address, port, interface and pairing state.
    MAA_TOOLKIT_API const char* MaaToolkitIOSDeviceGetConnections(const MaaToolkitIOSDevice* device);
    MAA_TOOLKIT_API MaaIOScreencapMethod MaaToolkitIOSDeviceGetScreencapMethods(const MaaToolkitIOSDevice* device);
    MAA_TOOLKIT_API const char* MaaToolkitIOSDeviceListGetWarnings(const MaaToolkitIOSDeviceList* list);

#ifdef __cplusplus
}
#endif
