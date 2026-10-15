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
     * udid selects the device; empty selects the only eligible device.
     * wifi_address empty prefers USB, then paired Wi-Fi without a USB candidate.
     * "auto" discovers the paired device over mDNS;
     * an explicit IPv4/IPv6 address uses its RemotePairing service directly.
     * Pairing is performed separately with MaaIOSManager, and records are read
     * from MaaFramework/iOS under XDG_DATA_HOME or the platform user data directory.
     * Development services require a suitable mounted Developer Disk Image;
     * MaaIOSManager prepare downloads and mounts one without installing Xcode.
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
        MaaIOSControlUnitCreate(const char* udid, const char* wifi_address, MaaIOScreencapMethod screencap_methods);

    MAA_CONTROL_UNIT_API void MaaIOSControlUnitDestroy(MaaIOSControlUnitHandle handle);

#ifdef __cplusplus
}
#endif
