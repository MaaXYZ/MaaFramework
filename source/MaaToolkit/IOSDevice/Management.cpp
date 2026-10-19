#include "MaaToolkit/IOSDevice/MaaToolkitIOSDevice.h"

#include <memory>

#include "ControlUnit/IOSDeviceAPI.h"
#include "LibraryHolder/ControlUnit.h"
#include "MaaFramework/Utility/MaaBuffer.h"
#include "MaaUtils/ScopeLeave.hpp"

namespace
{
using DeviceAPI = MAA_CTRL_UNIT_NS::IOSDeviceAPI;

struct OwnedPairing : MaaIOSWiFiPairing
{
    OwnedPairing(DeviceAPI* api, MaaIOSWiFiPairing* pairing)
        : MaaIOSWiFiPairing(*pairing)
        , api_(api)
        , pairing_(pairing)
    {
    }

    ~OwnedPairing() { api_->destroy_pairing(pairing_); }

private:
    DeviceAPI* api_;
    MaaIOSWiFiPairing* pairing_;
};

MaaIOSWiFiPairing* own_pairing(DeviceAPI* api, MaaIOSWiFiPairing* pairing)
{
    if (!pairing) {
        return nullptr;
    }
    OnScopeLeave([&]() {
        if (pairing) {
            api->destroy_pairing(pairing);
        }
    });
    auto result = std::make_unique<OwnedPairing>(api, pairing);
    pairing = nullptr;
    return result.release();
}

template <typename Function>
MaaBool invoke(MaaStringBuffer* result, Function action)
{
    if (!result) {
        return false;
    }
    try {
        auto api = MAA_NS::IOSControlUnitLibraryHolder::device_api();
        if (api) {
            return action(api);
        }
        MaaStringBufferSet(result, R"({"success":false,"cancelled":false,"error":"Failed to load iOS device management"})");
    }
    catch (...) {
        MaaStringBufferSet(result, R"({"success":false,"cancelled":false,"error":"iOS device management failed"})");
    }
    return false;
}
} // namespace

MaaBool MaaToolkitIOSDeviceDiscover(
    MaaIOSConnectionMethod connection_method,
    const MaaIOSWiFiPairing* const* wifi_pairings,
    MaaSize wifi_pairing_count,
    const char* options,
    MaaToolkitIOSDeviceEventCallback event,
    MaaToolkitIOSDeviceCancelCallback cancel,
    void* userdata,
    MaaStringBuffer* result)
{
    return invoke(result, [&](const auto& api) {
        return api->discover(connection_method, wifi_pairings, wifi_pairing_count, options, event, cancel, userdata, result);
    });
}

MaaBool MaaToolkitIOSDevicePair(
    const char* udid,
    MaaIOSConnectionMethod connection_method,
    const char* options,
    MaaToolkitIOSDeviceEventCallback event,
    MaaToolkitIOSDeviceCancelCallback cancel,
    void* userdata,
    MaaStringBuffer* result,
    MaaIOSWiFiPairing** credential)
{
    if (!credential) {
        return false;
    }
    *credential = nullptr;
    return invoke(result, [&](const auto& api) {
        MaaIOSWiFiPairing* pairing = nullptr;
        OnScopeLeave([&]() {
            if (pairing) {
                api->destroy_pairing(pairing);
            }
        });
        const auto success = api->pair(udid, connection_method, options, event, cancel, userdata, result, &pairing);
        if (success && pairing) {
            auto* transferred = pairing;
            pairing = nullptr;
            *credential = own_pairing(api, transferred);
        }
        return success;
    });
}

MaaBool MaaToolkitIOSDeviceCheckConnection(
    const char* udid,
    MaaIOSConnectionMethod connection_method,
    const char* wifi_address,
    const MaaIOSWiFiPairing* wifi_pairing,
    const char* options,
    MaaToolkitIOSDeviceEventCallback event,
    MaaToolkitIOSDeviceCancelCallback cancel,
    void* userdata,
    MaaStringBuffer* result)
{
    return invoke(result, [&](const auto& api) {
        return api->check_connection(udid, connection_method, wifi_address, wifi_pairing, options, event, cancel, userdata, result);
    });
}

MaaBool MaaToolkitIOSDevicePrepare(
    const char* udid,
    MaaIOSConnectionMethod connection_method,
    const char* wifi_address,
    const MaaIOSWiFiPairing* wifi_pairing,
    const char* options,
    MaaToolkitIOSDeviceEventCallback event,
    MaaToolkitIOSDeviceCancelCallback cancel,
    void* userdata,
    MaaStringBuffer* result)
{
    return invoke(result, [&](const auto& api) {
        return api->prepare(udid, connection_method, wifi_address, wifi_pairing, options, event, cancel, userdata, result);
    });
}

MaaIOSWiFiPairing* MaaToolkitIOSDeviceParsePairingRecord(const char* record)
{
    if (!record) {
        return nullptr;
    }
    try {
        auto api = MAA_NS::IOSControlUnitLibraryHolder::device_api();
        return api ? own_pairing(api, api->parse_pairing_record(record)) : nullptr;
    }
    catch (...) {
        return nullptr;
    }
}

MaaBool MaaToolkitIOSDeviceFormatPairingRecord(const MaaIOSWiFiPairing* pairing, MaaStringBuffer* record)
{
    if (!record) {
        return false;
    }
    MaaStringBufferClear(record);
    try {
        auto api = MAA_NS::IOSControlUnitLibraryHolder::device_api();
        return api && api->format_pairing_record(pairing, record);
    }
    catch (...) {
        return false;
    }
}

void MaaToolkitIOSDeviceDestroyPairing(MaaIOSWiFiPairing* pairing)
{
    delete static_cast<OwnedPairing*>(pairing);
}
