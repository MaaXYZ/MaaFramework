#include "MaaToolkit/IOSDevice/MaaToolkitIOSDevice.h"
#include "remote/Discovery.h"
#include <iostream>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>

namespace
{
maa::ios::remote::DiscoveryResult next_result;
bool throw_discovery = false;

void require(bool value, const char* message)
{
    if (!value) {
        throw std::runtime_error(message);
    }
}
}

namespace maa::ios::remote
{
DiscoveryResult discover_devices(const DiscoveryOptions&)
{
    if (throw_discovery) {
        throw std::runtime_error("private protocol failure");
    }
    return next_result;
}
}

int main()
{
    try {
        using namespace maa::ios::remote;
        auto* list = MaaToolkitIOSDeviceListCreate();
        require(list != nullptr, "allocate");
        next_result.usb_available = true;
        next_result.wifi_available = true;
        DiscoveryCandidate usb;
        usb.connection_type = "Network";
        DiscoveryCandidate wifi;
        wifi.transport = DiscoveryTransport::remote_pairing;
        wifi.address = "fe80::1%14";
        wifi.port = 49153;
        wifi.interface_index = 14;
        wifi.interface_name = "en0";
        wifi.pairing = DiscoveryPairing::ready;
        next_result.devices.push_back({ "device-id", "phone", { usb, wifi } });
        next_result.warnings.push_back("one source unavailable");
        require(MaaToolkitIOSDeviceFindWithTimeout(5, list), "partial success");
        const auto* device = MaaToolkitIOSDeviceListAt(list, 0);
        require(device && MaaToolkitIOSDeviceListSize(list) == 1, "list view");
        std::string owned_udid = MaaToolkitIOSDeviceGetUdid(device);
        const auto candidates = nlohmann::json::parse(MaaToolkitIOSDeviceGetConnections(device));
        require(candidates.size() == 2 && candidates[0]["connection_type"] == "Network", "preserve usbmux network");
        require(
            candidates[1]["address"] == "fe80::1%14" && candidates[1]["port"] == 49153 && candidates[1]["interface_index"] == 14,
            "preserve scoped wifi endpoint");
        require(nlohmann::json::parse(MaaToolkitIOSDeviceListGetWarnings(list)).size() == 1, "partial warnings");
        require(
            !MaaToolkitIOSDeviceFindWithTimeout(-1, list) && MaaToolkitIOSDeviceListSize(list) == 0,
            "invalid refresh clears old borrowed data");
        throw_discovery = true;
        require(!MaaToolkitIOSDeviceFind(list) && MaaToolkitIOSDeviceListSize(list) == 0, "C boundary catches discovery exception");
        require(
            std::string(MaaToolkitIOSDeviceListGetWarnings(list)).find("private protocol failure") == std::string::npos,
            "does not leak protocol exception data");
        MaaToolkitIOSDeviceListDestroy(list);
        require(owned_udid == "device-id", "caller copy outlives list");
        require(!MaaToolkitIOSDeviceFind(nullptr) && MaaToolkitIOSDeviceListAt(nullptr, 0) == nullptr, "null handles");
        std::cout << "iOS toolkit boundary passed\n";
        return 0;
    }
    catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
