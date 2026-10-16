#include "MaaToolkit/IOSDevice/MaaToolkitIOSDevice.h"

#include "LibraryHolder/ControlUnit.h"
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
};

struct MaaToolkitIOSDeviceList
{
    std::vector<MaaToolkitIOSDevice> devices;
    std::string warnings = "[]";
};

namespace
{
MaaBool manage(
    const char* function_name,
    const char* options_json,
    MaaToolkitIOSDeviceEventCallback event_callback,
    MaaToolkitIOSDeviceCancelCallback cancel_callback,
    void* userdata,
    MaaStringBuffer* result,
    MaaStringBuffer* credential = nullptr)
{
    if (!result) {
        return false;
    }
    try {
        if (credential) {
            MaaStringBufferClear(credential);
        }
#ifdef MAA_WITH_IOS_DEVICE
        return MAA_NS::IOSControlUnitLibraryHolder::management(
            function_name,
            options_json,
            event_callback,
            cancel_callback,
            userdata,
            result,
            credential);
#else
        (void)function_name;
        (void)options_json;
        (void)event_callback;
        (void)cancel_callback;
        (void)userdata;
        MaaStringBufferSet(result, R"({"success":false,"cancelled":false,"error":"iOS controller support is disabled in this build"})");
        return false;
#endif
    }
    catch (...) {
        MaaStringBufferSet(result, R"({"success":false,"cancelled":false,"error":"iOS device management failed"})");
        return false;
    }
}
}

MaaBool MaaToolkitIOSDeviceDiscover(
    const char* options_json,
    MaaToolkitIOSDeviceEventCallback event_callback,
    MaaToolkitIOSDeviceCancelCallback cancel_callback,
    void* userdata,
    MaaStringBuffer* result)
{
    return manage("MaaIOSControlUnitDiscover", options_json, event_callback, cancel_callback, userdata, result);
}

MaaBool MaaToolkitIOSDevicePair(
    const char* options_json,
    MaaToolkitIOSDeviceEventCallback event_callback,
    MaaToolkitIOSDeviceCancelCallback cancel_callback,
    void* userdata,
    MaaStringBuffer* result,
    MaaStringBuffer* credential)
{
    if (!credential || result == credential) {
        return false;
    }
    return manage("MaaIOSControlUnitPair", options_json, event_callback, cancel_callback, userdata, result, credential);
}

MaaBool MaaToolkitIOSDeviceCheckConnection(
    const char* options_json,
    MaaToolkitIOSDeviceEventCallback event_callback,
    MaaToolkitIOSDeviceCancelCallback cancel_callback,
    void* userdata,
    MaaStringBuffer* result)
{
    return manage("MaaIOSControlUnitCheckConnection", options_json, event_callback, cancel_callback, userdata, result);
}

MaaBool MaaToolkitIOSDevicePrepare(
    const char* options_json,
    MaaToolkitIOSDeviceEventCallback event_callback,
    MaaToolkitIOSDeviceCancelCallback cancel_callback,
    void* userdata,
    MaaStringBuffer* result)
{
    return manage("MaaIOSControlUnitPrepare", options_json, event_callback, cancel_callback, userdata, result);
}

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
                  device.get("candidates", json::array { }).to_string() });
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
#if defined(__ANDROID__)
    return device ? MaaIOScreencapMethod_ScreenshotService : MaaIOScreencapMethod_None;
#else
    return device ? (MaaIOScreencapMethod_Stream | MaaIOScreencapMethod_ScreenshotService) : MaaIOScreencapMethod_None;
#endif
}

const char* MaaToolkitIOSDeviceListGetWarnings(const MaaToolkitIOSDeviceList* list)
{
    return list ? list->warnings.c_str() : "[]";
}
