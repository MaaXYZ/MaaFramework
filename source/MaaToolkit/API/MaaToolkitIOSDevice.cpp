#include "MaaToolkit/IOSDevice/MaaToolkitIOSDevice.h"

#include <chrono>
#include <string>
#include <vector>
#ifdef MAA_WITH_IOS_DEVICE
#include "remote/Discovery.h"
#include <nlohmann/json.hpp>
#endif

struct MaaToolkitIOSDevice
{
    std::string name;
    std::string udid;
    std::string connections;
};

struct MaaToolkitIOSDeviceList
{
    std::vector<MaaToolkitIOSDevice> devices;
    std::string warnings = "[]";
};

MaaToolkitIOSDeviceList* MaaToolkitIOSDeviceListCreate()
{
    try {
        return new MaaToolkitIOSDeviceList;
    }
    catch (...) {
        return nullptr;
    }
}

void MaaToolkitIOSDeviceListDestroy(MaaToolkitIOSDeviceList* list)
{
    delete list;
}

MaaBool MaaToolkitIOSDeviceFind(MaaToolkitIOSDeviceList* list)
{
    return MaaToolkitIOSDeviceFindWithTimeout(3000, list);
}

MaaBool MaaToolkitIOSDeviceFindWithTimeout(int64_t timeout_ms, MaaToolkitIOSDeviceList* list)
{
    if (!list) {
        return false;
    }
    list->devices.clear();
    list->warnings = "[]";
    if (timeout_ms < 0 || timeout_ms > 60000) {
        list->warnings = "[\"timeout_ms must be between 0 and 60000\"]";
        return false;
    }
#ifdef MAA_WITH_IOS_DEVICE
    try {
        maa::ios::remote::DiscoveryOptions options;
        options.timeout = std::chrono::milliseconds(timeout_ms);
        auto result = maa::ios::remote::discover_devices(options);
        std::vector<MaaToolkitIOSDevice> devices;
        for (const auto& device : result.devices) {
            auto connections = nlohmann::json::array();
            for (const auto& c : device.candidates) {
                connections.push_back(
                    { { "transport", c.transport == maa::ios::remote::DiscoveryTransport::usbmux ? "usbmux" : "remote_pairing" },
                      { "connection_type", c.connection_type },
                      { "address", c.address },
                      { "port", c.port },
                      { "interface_index", c.interface_index },
                      { "interface_name", c.interface_name },
                      { "instance", c.instance },
                      { "identifier", c.identifier },
                      { "pairing_state", static_cast<int>(c.pairing) } });
            }
            devices.push_back({ device.name, device.udid, connections.dump() });
        }
        list->warnings = nlohmann::json(result.warnings).dump();
        list->devices = std::move(devices);
        return !result.cancelled && (result.usb_available || result.wifi_available);
    }
    catch (...) {
        list->devices.clear();
        list->warnings = "[\"iOS device discovery failed\"]";
        return false;
    }
#else
    list->warnings = "[\"iOS controller support is disabled in this build\"]";
    return false;
#endif
}

MaaSize MaaToolkitIOSDeviceListSize(const MaaToolkitIOSDeviceList* list)
{
    return list ? list->devices.size() : 0;
}

const MaaToolkitIOSDevice* MaaToolkitIOSDeviceListAt(const MaaToolkitIOSDeviceList* list, MaaSize index)
{
    return list && index < list->devices.size() ? &list->devices[index] : nullptr;
}

const char* MaaToolkitIOSDeviceGetName(const MaaToolkitIOSDevice* device)
{
    return device ? device->name.c_str() : "";
}

const char* MaaToolkitIOSDeviceGetUdid(const MaaToolkitIOSDevice* device)
{
    return device ? device->udid.c_str() : "";
}

const char* MaaToolkitIOSDeviceGetConnections(const MaaToolkitIOSDevice* device)
{
    return device ? device->connections.c_str() : "[]";
}

MaaIOScreencapMethod MaaToolkitIOSDeviceGetScreencapMethods(const MaaToolkitIOSDevice* device)
{
    return device ? (MaaIOScreencapMethod_Stream | MaaIOScreencapMethod_ScreenshotService) : MaaIOScreencapMethod_None;
}

const char* MaaToolkitIOSDeviceListGetWarnings(const MaaToolkitIOSDeviceList* list)
{
    return list ? list->warnings.c_str() : "[]";
}
