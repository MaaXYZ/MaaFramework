#pragma once

#include "MaaControlUnit/MaaControlUnitDef.h"
#include "MaaToolkit/IOSDevice/MaaToolkitIOSDevice.h"

MAA_CTRL_UNIT_NS_BEGIN

class IOSDeviceAPI
{
public:
    virtual ~IOSDeviceAPI() = default;

    virtual MaaBool discover(
        MaaIOSConnectionMethod connection_method,
        const MaaIOSWiFiPairing* const* wifi_pairings,
        MaaSize wifi_pairing_count,
        const char* options_json,
        MaaToolkitIOSDeviceEventCallback event_callback,
        MaaToolkitIOSDeviceCancelCallback cancel_callback,
        void* userdata,
        MaaStringBuffer* result) = 0;

    virtual MaaBool pair(
        const char* udid,
        MaaIOSConnectionMethod connection_method,
        const char* options_json,
        MaaToolkitIOSDeviceEventCallback event_callback,
        MaaToolkitIOSDeviceCancelCallback cancel_callback,
        void* userdata,
        MaaStringBuffer* result,
        MaaIOSWiFiPairing** pairing) = 0;

    virtual MaaBool check_connection(
        const char* udid,
        MaaIOSConnectionMethod connection_method,
        const char* wifi_address,
        const MaaIOSWiFiPairing* wifi_pairing,
        const char* options_json,
        MaaToolkitIOSDeviceEventCallback event_callback,
        MaaToolkitIOSDeviceCancelCallback cancel_callback,
        void* userdata,
        MaaStringBuffer* result) = 0;

    virtual MaaBool prepare(
        const char* udid,
        MaaIOSConnectionMethod connection_method,
        const char* wifi_address,
        const MaaIOSWiFiPairing* wifi_pairing,
        const char* options_json,
        MaaToolkitIOSDeviceEventCallback event_callback,
        MaaToolkitIOSDeviceCancelCallback cancel_callback,
        void* userdata,
        MaaStringBuffer* result) = 0;

    virtual MaaIOSWiFiPairing* parse_pairing_record(const char* record) = 0;

    virtual MaaBool format_pairing_record(const MaaIOSWiFiPairing* pairing, MaaStringBuffer* record) = 0;

    virtual void destroy_pairing(MaaIOSWiFiPairing* pairing) = 0;
};

MAA_CTRL_UNIT_NS_END

// Borrowed stateless implementation; its caller must retain the loaded module.
extern "C" MAA_CONTROL_UNIT_API MAA_CTRL_UNIT_NS::IOSDeviceAPI* MaaIOSControlUnitGetDeviceAPI();
