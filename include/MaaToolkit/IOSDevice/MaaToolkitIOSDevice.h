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
    typedef MaaBool (*MaaToolkitIOSDeviceEventCallback)(const char* event_json, void* userdata);
    typedef MaaBool (*MaaToolkitIOSDeviceCancelCallback)(void* userdata);

    // Options: timeout_ms (0..60000, default 3000).
    // Auto discovers USB and Wi-Fi; USB and WiFi restrict discovery to that transport.
    // Inputs are borrowed until return and copied before device I/O.
    MAA_TOOLKIT_API MaaBool MaaToolkitIOSDeviceDiscover(
        MaaIOSConnectionMethod connection_method,
        const MaaIOSWiFiPairing* const* wifi_pairings,
        MaaSize wifi_pairing_count,
        const char* options_json,
        MaaToolkitIOSDeviceEventCallback event_callback,
        MaaToolkitIOSDeviceCancelCallback cancel_callback,
        void* userdata,
        MaaStringBuffer* result);

    // Options: timeout_ms (1..300000, default 120000), host_name (Wi-Fi advertisement).
    // Auto prefers physical USB, then Wi-Fi pairing. Wi-Fi requires a PIN event callback.
    // Both transports create a new RemotePairing credential for subsequent Wi-Fi connections.
    // USB opens the pairing channel through usbmux using the system-managed USB trust record.
    // result is a caller-created status buffer. pairing is a non-NULL output pointer;
    // success returns an owned structured credential, failure sets *pairing to NULL.
    // Release it with MaaToolkitIOSDeviceDestroyPairing. Its field pointers remain valid
    // until destruction. Persistence and access control belong to the caller.
    MAA_TOOLKIT_API MaaBool MaaToolkitIOSDevicePair(
        const char* udid,
        MaaIOSConnectionMethod connection_method,
        const char* options_json,
        MaaToolkitIOSDeviceEventCallback event_callback,
        MaaToolkitIOSDeviceCancelCallback cancel_callback,
        void* userdata,
        MaaStringBuffer* result,
        MaaIOSWiFiPairing** pairing);

    // Optional persistence using the versioned "MaaIOS1" text format.
    // Parse returns an owned object; destroy only objects returned by Parse or Pair.
    // Export accepts a caller-assembled view as well. These functions perform no device I/O.
    MAA_TOOLKIT_API MaaIOSWiFiPairing* MaaToolkitIOSDeviceParsePairingRecord(const char* record);
    MAA_TOOLKIT_API MaaBool MaaToolkitIOSDeviceFormatPairingRecord(const MaaIOSWiFiPairing* pairing, MaaStringBuffer* record);
    MAA_TOOLKIT_API void MaaToolkitIOSDeviceDestroyPairing(MaaIOSWiFiPairing* pairing);

    // Options: port (explicit Wi-Fi address only).
    // Auto prefers physical USB, then Wi-Fi. WiFi requires wifi_pairing.
    // A nonempty UDID is required and must match wifi_pairing when provided.
    // Empty/"auto" Wi-Fi address enables discovery. USB ignores Wi-Fi inputs.
    // Closes the verified session before returning; does not create a shared tunnel.
    MAA_TOOLKIT_API MaaBool MaaToolkitIOSDeviceCheckConnection(
        const char* udid,
        MaaIOSConnectionMethod connection_method,
        const char* wifi_address,
        const MaaIOSWiFiPairing* wifi_pairing,
        const char* options_json,
        MaaToolkitIOSDeviceEventCallback event_callback,
        MaaToolkitIOSDeviceCancelCallback cancel_callback,
        void* userdata,
        MaaStringBuffer* result);

    // A nonempty UDID is required, using the same connection rules as CheckConnection.
    // Options: port (explicit Wi-Fi address only), cache_directory, replace (default false),
    // download_timeout_seconds (1..3600, default 300),
    // install_timeout_seconds (1..3600, default 180).
    // Empty cache_directory uses temporary files and cleans them after the operation;
    // only an explicit directory enables persistent caching. Failed/cancelled preparation
    // may have changed the device; inspect device_may_have_changed before replacing it.
    MAA_TOOLKIT_API MaaBool MaaToolkitIOSDevicePrepare(
        const char* udid,
        MaaIOSConnectionMethod connection_method,
        const char* wifi_address,
        const MaaIOSWiFiPairing* wifi_pairing,
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
        const MaaIOSWiFiPairing* const* wifi_pairings,
        MaaSize wifi_pairing_count,
        const char* options_json,
        MaaToolkitIOSDeviceList* list);
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
