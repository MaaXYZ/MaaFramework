#include "MaaFramework/Utility/MaaBuffer.h"
#include "MaaToolkit/IOSDevice/MaaToolkitIOSDevice.h"

#include <iostream>
#include <meojson/json.hpp>
#include <stdexcept>
#include <string>

namespace
{
std::string next_result;
bool discovery_succeeded = true;
bool discovery_called = false;

void require(bool value, const char* message)
{
    if (!value) {
        throw std::runtime_error(message);
    }
}
}

MaaBool MaaToolkitIOSDeviceDiscover(
    MaaIOSConnectionMethod,
    const MaaIOSWiFiPairing* const*,
    MaaSize,
    const char*,
    MaaToolkitIOSDeviceEventCallback,
    MaaToolkitIOSDeviceCancelCallback,
    void*,
    MaaStringBuffer* result)
{
    discovery_called = true;
    MaaStringBufferSet(result, next_result.c_str());
    return discovery_succeeded;
}

int main()
{
    try {
        auto* list = MaaToolkitIOSDeviceListCreate();
        require(list != nullptr, "allocate");
        next_result = R"({"success":true,"cancelled":false,"error":"","usb_available":true,"wifi_available":true,
            "devices":[{"udid":"device-id","name":"phone","screencap_methods":3,"candidates":[
                {"transport":"usbmux","connection_type":"Network"},
                {"transport":"remote_pairing","address":"fe80::1%14","port":49153,"interface_index":14,"pairing_state":1}]}],
            "warnings":["one source unavailable"]})";
        require(MaaToolkitIOSDeviceFindWithTimeout(5, list), "partial success");
        require(discovery_called, "device list uses Toolkit discovery");
        const auto* device = MaaToolkitIOSDeviceListAt(list, 0);
        require(device && MaaToolkitIOSDeviceListSize(list) == 1, "list view");
        require(
            MaaToolkitIOSDeviceGetScreencapMethods(device) == (MaaIOSScreencapMethod_Stream | MaaIOSScreencapMethod_ScreenshotService),
            "report decoder capability supplied by Toolkit discovery");
        require(MaaToolkitIOSDeviceGetScreencapMethods(nullptr) == MaaIOSScreencapMethod_None, "null device has no capture methods");
        const auto with_video = next_result;
        next_result = R"({"success":true,"devices":[{"udid":"screenshot-only","screencap_methods":2}]})";
        require(MaaToolkitIOSDeviceFindWithTimeout(0, list), "screenshot-only discovery");
        require(
            MaaToolkitIOSDeviceGetScreencapMethods(MaaToolkitIOSDeviceListAt(list, 0)) == MaaIOSScreencapMethod_ScreenshotService,
            "do not advertise video without a decoder");
        next_result = with_video;
        require(MaaToolkitIOSDeviceFindWithTimeout(5, list), "restore video-capable discovery");
        device = MaaToolkitIOSDeviceListAt(list, 0);
        std::string owned_udid = MaaToolkitIOSDeviceGetUdid(device);
        const auto candidates = json::parse(MaaToolkitIOSDeviceGetConnections(device));
        require(candidates && candidates->as_array().size() == 2, "retain candidates");
        require((*candidates)[0].get("connection_type", std::string { }) == "Network", "preserve usbmux network");
        require(
            (*candidates)[1].get("address", std::string { }) == "fe80::1%14" && (*candidates)[1].get("port", 0) == 49153
                && (*candidates)[1].get("interface_index", 0) == 14,
            "preserve scoped wifi endpoint");
        require(json::parse(MaaToolkitIOSDeviceListGetWarnings(list))->as_array().size() == 1, "partial warnings");
        require(
            !MaaToolkitIOSDeviceFindWithTimeout(-1, list) && MaaToolkitIOSDeviceListSize(list) == 0,
            "invalid refresh clears old borrowed data");
        require(MaaToolkitIOSDeviceFindWithTimeout(0, list), "restore discovery before failure");
        discovery_succeeded = false;
        next_result = R"({"success":false,"error":"discovery unavailable"})";
        require(!MaaToolkitIOSDeviceFind(list) && MaaToolkitIOSDeviceListSize(list) == 0, "failed discovery clears device list");
        require(
            json::parse(MaaToolkitIOSDeviceListGetWarnings(list))->as_array()[0].as_string() == "discovery unavailable",
            "report discovery failure");
        discovery_succeeded = true;
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
