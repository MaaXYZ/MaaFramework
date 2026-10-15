#pragma once

#include "MaaControlUnit/ControlUnitAPI.h"
#include "MaaFramework/MaaDef.h"

#ifdef __cplusplus
extern "C"
{
#endif

    MAA_CONTROL_UNIT_API const char* MaaIOSControlUnitGetVersion();

    /**
     * Synchronous device-management events. Return false to reject a PIN or
     * request cancellation. event_json is borrowed only during this callback.
     * Events have type "progress" or "pin"; a PIN is temporary pairing material.
     * Event callbacks are serialized. Cancel callbacks may run on worker/watchdog
     * threads and must be thread-safe. Callbacks must not throw or reenter the call.
     */
    typedef MaaBool (*MaaIOSControlUnitManagementEventCallback)(const char* event_json, void* userdata);
    typedef MaaBool (*MaaIOSControlUnitManagementCancelCallback)(void* userdata);

    /**
     * Blocking device management. options_json is an object (NULL means {}).
     * result is a caller-owned MaaStringBuffer and must remain alive until return.
     * Results always contain success, error and cancelled, including failures.
     * Callbacks/userdata are borrowed only until return; no jobs remain afterward.
     * Wireless first pairing requires an event callback to present its one-use PIN.
     *
     * Discover: timeout_ms (0..60000, default 3000), usb_only, wifi_only.
     * Pair: udid, wifi (empty or "auto"), repair, timeout_ms (1..300000, default 120000).
     * CheckConnection: udid, wifi (empty/"auto"/address), port (1..65535).
     * Prepare: connection fields, cache_directory, replace, download_timeout_seconds
     *          and install_timeout_seconds (1..3600; defaults 300 and 180).
     * Unknown fields are rejected. CheckConnection closes its session before return.
     * Failed/cancelled preparation can have changed the device; inspect
     * device_may_have_changed before retrying with replacement.
     */
    MAA_CONTROL_UNIT_API MaaBool MaaIOSControlUnitDiscover(
        const char* options_json,
        MaaIOSControlUnitManagementEventCallback event_callback,
        MaaIOSControlUnitManagementCancelCallback cancel_callback,
        void* userdata,
        MaaStringBuffer* result);
    MAA_CONTROL_UNIT_API MaaBool MaaIOSControlUnitPair(
        const char* options_json,
        MaaIOSControlUnitManagementEventCallback event_callback,
        MaaIOSControlUnitManagementCancelCallback cancel_callback,
        void* userdata,
        MaaStringBuffer* result,
        MaaStringBuffer* credential);
    MAA_CONTROL_UNIT_API MaaBool MaaIOSControlUnitCheckConnection(
        const char* options_json,
        MaaIOSControlUnitManagementEventCallback event_callback,
        MaaIOSControlUnitManagementCancelCallback cancel_callback,
        void* userdata,
        MaaStringBuffer* result);
    MAA_CONTROL_UNIT_API MaaBool MaaIOSControlUnitPrepare(
        const char* options_json,
        MaaIOSControlUnitManagementEventCallback event_callback,
        MaaIOSControlUnitManagementCancelCallback cancel_callback,
        void* userdata,
        MaaStringBuffer* result);

    /**
     * Create a native MaaFramework iOS controller.
     *
     * udid selects the device; empty selects the only eligible device.
     * wifi_address empty prefers USB, then paired Wi-Fi without a USB candidate.
     * "auto" discovers the paired device over mDNS;
     * an explicit IPv4/IPv6 address uses its RemotePairing service directly.
     * Pairing is performed separately through MaaToolkit. config is JSON with
     * pairing_records: an array of caller-owned opaque credential strings.
     * The controller owns a copy; no implicit pairing directory is read or written.
     * Development services require a suitable mounted Developer Disk Image;
     * MaaToolkitIOSDevicePrepare downloads and mounts one without installing Xcode.
     *
     * screencap_methods is a bitmask: ScreenshotService makes lossless PNG RPCs
     * without starting video; Stream receives and decodes HEVC frames. Combining
     * both permits fallback to ScreenshotService when video is unavailable.
     * Touch coordinates use the pixels and geometry of the last delivered image.
     *
     * Keys use Android keycodes. Standard keyboard keys and the supported home,
     * power, volume and mute buttons support click_key, key_down and key_up.
     * Unsupported keys, relative movement and additional touch contacts fail.
     * input_text supports nonempty US-layout ASCII, tab and newline; strings
     * containing unsupported characters are rejected before sending any key.
     * Device-side input methods determine the interpretation of physical keys.
     *
     * start_app brings an existing application to the foreground without killing
     * it first. stop_app stops all processes in its bundle and succeeds if none
     * were running. Both take an application bundle identifier.
     */
    MAA_CONTROL_UNIT_API MaaIOSControlUnitHandle
        MaaIOSControlUnitCreate(const char* udid, const char* wifi_address, MaaIOScreencapMethod screencap_methods, const char* config);

    MAA_CONTROL_UNIT_API void MaaIOSControlUnitDestroy(MaaIOSControlUnitHandle handle);

#ifdef __cplusplus
}
#endif
