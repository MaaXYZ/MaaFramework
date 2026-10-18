#include "MaaToolkit/IOSDevice/MaaToolkitIOSDevice.h"

#include "MaaFramework/Utility/MaaBuffer.h"
#include <memory>
#include <meojson/json.hpp>
#include <string>
#include <vector>

struct MaaToolkitIOSDevice
{
    std::string name;
    std::string udid;
    std::string connections;
    MaaIOScreencapMethod screencap_methods = MaaIOScreencapMethod_ScreenshotService;
};

struct MaaToolkitIOSDeviceList
{
    std::vector<MaaToolkitIOSDevice> devices;
    std::string warnings = "[]";
};

#ifndef MAA_WITH_IOS_DEVICE
namespace
{
MaaBool unsupported(MaaStringBuffer* result, MaaStringBuffer* credential = nullptr)
{
    if (!result || result == credential) {
        return false;
    }
    if (credential) {
        MaaStringBufferClear(credential);
    }
    MaaStringBufferSet(result, R"({"success":false,"cancelled":false,"error":"iOS controller support is disabled in this build"})");
    return false;
}
}

MaaBool MaaToolkitIOSDeviceDiscover(
    const char*,
    MaaToolkitIOSDeviceEventCallback,
    MaaToolkitIOSDeviceCancelCallback,
    void*,
    MaaStringBuffer* result)
{
    return unsupported(result);
}

MaaBool MaaToolkitIOSDevicePair(
    const char*,
    MaaToolkitIOSDeviceEventCallback,
    MaaToolkitIOSDeviceCancelCallback,
    void*,
    MaaStringBuffer* result,
    MaaStringBuffer* credential)
{
    return credential && unsupported(result, credential);
}

MaaBool MaaToolkitIOSDeviceCheckConnection(
    const char*,
    MaaToolkitIOSDeviceEventCallback,
    MaaToolkitIOSDeviceCancelCallback,
    void*,
    MaaStringBuffer* result)
{
    return unsupported(result);
}

MaaBool MaaToolkitIOSDevicePrepare(
    const char*,
    MaaToolkitIOSDeviceEventCallback,
    MaaToolkitIOSDeviceCancelCallback,
    void*,
    MaaStringBuffer* result)
{
    return unsupported(result);
}
#endif

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
    const auto options = json::value(json::object { { "timeout_ms", timeout_ms } }).to_string();
    return MaaToolkitIOSDeviceFindWithOptions(options.c_str(), list);
}

MaaBool MaaToolkitIOSDeviceFindWithOptions(const char* options_json, MaaToolkitIOSDeviceList* list)
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
        const bool success = MaaToolkitIOSDeviceDiscover(options_json, nullptr, nullptr, nullptr, result.get());
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
                  device.get("candidates", json::array { }).to_string(),
                  device.get("screencap_methods", MaaIOScreencapMethod_ScreenshotService) });
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

const char* MaaToolkitIOSDeviceGetConnections(const MaaToolkitIOSDevice* device)
{
    return device ? device->connections.c_str() : "[]";
}

MaaIOScreencapMethod MaaToolkitIOSDeviceGetScreencapMethods(const MaaToolkitIOSDevice* device)
{
    return device ? device->screencap_methods : MaaIOScreencapMethod_None;
}

const char* MaaToolkitIOSDeviceListGetWarnings(const MaaToolkitIOSDeviceList* list)
{
    return list ? list->warnings.c_str() : "[]";
}
