#pragma once

#include "MaaControlUnit/ControlUnitAPI.h"
#include "MaaFramework/MaaDef.h"

#ifdef __cplusplus
extern "C"
{
#endif

    MAA_CONTROL_UNIT_API const char* MaaIOSControlUnitGetVersion();

    /**
     * Create a native MaaFramework iOS controller.
     *
     * udid is required and must be nonempty; it selects the target device.
     * Auto prefers physical USB then RemotePairing; USB never connects wirelessly.
     * WiFi requires wifi_pairing. Empty wifi_address (or "auto") discovers the device;
     * an explicit IPv4/IPv6 address uses its RemotePairing service directly.
     * Pairing is performed separately through MaaToolkit. The controller copies raw
     * fields during creation. An explicit udid must match the wireless credential.
     * USB ignores wifi_address and wifi_pairing and uses system usbmux trust.
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
    MAA_CONTROL_UNIT_API MaaIOSControlUnitHandle MaaIOSControlUnitCreate(
        const char* udid,
        MaaIOSScreencapMethod screencap_methods,
        MaaIOSConnectionMethod connection_method,
        const char* wifi_address,
        const MaaIOSWiFiPairing* wifi_pairing);

    MAA_CONTROL_UNIT_API void MaaIOSControlUnitDestroy(MaaIOSControlUnitHandle handle);

#ifdef __cplusplus
}
#endif
