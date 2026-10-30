#include "MaaToolkit/IOSDevice/MaaToolkitIOSDevice.h"

#include "MaaFramework/Utility/MaaBuffer.h"
#include "ControlUnit/IOSDeviceAPI.h"
#include "LibraryHolder/ControlUnit.h"
#include <memory>
#include <meojson/json.hpp>
#include <string>
#include <vector>

struct MaaToolkitIOSDevice
{
    std::string name;
    std::string udid;
    std::string state;
    std::string connections;
    MaaIOSScreencapMethod screencap_methods = MaaIOSScreencapMethod_ScreenshotService;
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
    return MaaToolkitIOSDeviceFindWithOptions(MaaIOSConnectionMethod_Auto, nullptr, nullptr, nullptr, list);
}

MaaBool MaaToolkitIOSDeviceFindWithOptions(
    MaaIOSConnectionMethod connection_method,
    const char* options_json,
    MaaToolkitIOSDeviceCancelCallback cancel_callback,
    void* userdata,
    MaaToolkitIOSDeviceList* list)
{
    if (!list) {
        return false;
    }
    list->devices.clear();
    list->warnings = "[]";
    try {
        std::unique_ptr<MaaStringBuffer, decltype(&MaaStringBufferDestroy)> result(MaaStringBufferCreate(), MaaStringBufferDestroy);
        if (!result) {
            return false;
        }
        auto api = MAA_NS::IOSControlUnitLibraryHolder::device_api();
        if (!api) {
            list->warnings = "[\"Failed to load iOS device management\"]";
            return false;
        }
        const bool success = api->discover(connection_method, options_json, cancel_callback, userdata, result.get());
        const auto parsed = json::parse(MaaStringBufferGet(result.get()));
        if (!parsed || !parsed->is_object()) {
            list->warnings = "[\"invalid iOS discovery result\"]";
            return false;
        }
        list->warnings = parsed->get("warnings", json::array { }).to_string();
        if (!success) {
            const auto error = parsed->get("error", std::string { });
            if (!error.empty()) {
                list->warnings = json::value(json::array { error }).to_string();
            }
            return false;
        }
        for (const auto& device : parsed->get("devices", json::array { })) {
            list->devices.push_back(
                { device.get("name", std::string { }),
                  device.get("udid", std::string { }),
                  device.get("state", std::string("discovered")),
                  device.get("candidates", json::array { }).to_string(),
                  device.get("screencap_methods", MaaIOSScreencapMethod_ScreenshotService) });
        }
        return true;
    }
    catch (...) {
        list->devices.clear();
        list->warnings = "[\"iOS device discovery failed\"]";
        return false;
    }
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

const char* MaaToolkitIOSDeviceGetState(const MaaToolkitIOSDevice* device)
{
    return device ? device->state.c_str() : "";
}

const char* MaaToolkitIOSDeviceGetConnections(const MaaToolkitIOSDevice* device)
{
    return device ? device->connections.c_str() : "[]";
}

MaaIOSScreencapMethod MaaToolkitIOSDeviceGetScreencapMethods(const MaaToolkitIOSDevice* device)
{
    return device ? device->screencap_methods : MaaIOSScreencapMethod_None;
}

const char* MaaToolkitIOSDeviceListGetWarnings(const MaaToolkitIOSDeviceList* list)
{
    return list ? list->warnings.c_str() : "[]";
}
