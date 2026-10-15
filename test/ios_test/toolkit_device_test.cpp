#include "LibraryHolder/ControlUnit.h"
#include "MaaFramework/Utility/MaaBuffer.h"
#include "MaaToolkit/IOSDevice/MaaToolkitIOSDevice.h"

#include <iostream>
#include <meojson/json.hpp>
#include <stdexcept>
#include <string>

namespace
{
std::string next_result;
bool throw_management = false;
std::string last_function;

void require(bool value, const char* message)
{
    if (!value) {
        throw std::runtime_error(message);
    }
}
}

MAA_NS_BEGIN
MaaBool IOSControlUnitLibraryHolder::management(
    const char* function_name,
    const char*,
    MaaIOSControlUnitManagementEventCallback event_callback,
    MaaIOSControlUnitManagementCancelCallback cancel_callback,
    void* userdata,
    MaaStringBuffer* result,
    MaaStringBuffer* credential)
{
    if (throw_management) {
        throw std::runtime_error("private protocol failure");
    }
    last_function = function_name;
    if (event_callback && !event_callback(R"({"type":"pin","pin":"123456"})", userdata)) {
        return false;
    }
    if (cancel_callback && cancel_callback(userdata)) {
        return false;
    }
    MaaStringBufferSet(result, next_result.c_str());
    if (credential) {
        MaaStringBufferSet(credential, "opaque-test-credential");
    }
    return true;
}

MAA_NS_END

int main()
{
    try {
        auto* list = MaaToolkitIOSDeviceListCreate();
        require(list != nullptr, "allocate");
        next_result = R"({"success":true,"cancelled":false,"error":"","usb_available":true,"wifi_available":true,
            "devices":[{"udid":"device-id","name":"phone","candidates":[
                {"transport":"usbmux","connection_type":"Network"},
                {"transport":"remote_pairing","address":"fe80::1%14","port":49153,"interface_index":14,"pairing_state":1}]}],
            "warnings":["one source unavailable"]})";
        require(MaaToolkitIOSDeviceFindWithTimeout(5, list), "partial success");
        require(last_function == "MaaIOSControlUnitDiscover", "discovery uses control unit");
        const auto* device = MaaToolkitIOSDeviceListAt(list, 0);
        require(device && MaaToolkitIOSDeviceListSize(list) == 1, "list view");
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
        throw_management = true;
        require(!MaaToolkitIOSDeviceFind(list) && MaaToolkitIOSDeviceListSize(list) == 0, "C boundary catches management exception");
        require(
            std::string(MaaToolkitIOSDeviceListGetWarnings(list)).find("private protocol failure") == std::string::npos,
            "does not leak protocol exception data");
        throw_management = false;
        auto* result = MaaStringBufferCreate();
        auto* credential = MaaStringBufferCreate();
        require(result != nullptr, "result allocation");
        require(
            MaaToolkitIOSDevicePair("{}", nullptr, nullptr, nullptr, result, credential) && last_function == "MaaIOSControlUnitPair",
            "pair uses control unit");
        require(
            MaaToolkitIOSDeviceCheckConnection("{}", nullptr, nullptr, nullptr, result)
                && last_function == "MaaIOSControlUnitCheckConnection",
            "connection uses control unit");
        require(
            MaaToolkitIOSDevicePrepare("{}", nullptr, nullptr, nullptr, result) && last_function == "MaaIOSControlUnitPrepare",
            "DDI uses control unit");
        bool pin_seen = false;
        require(
            MaaToolkitIOSDevicePair(
                "{}",
                [](const char* value, void* data) -> MaaBool {
                    *static_cast<bool*>(data) = std::string(value).find("123456") != std::string::npos;
                    return true;
                },
                nullptr,
                &pin_seen,
                result,
                credential)
                && pin_seen,
            "pairing events reach caller");
        require(
            !MaaToolkitIOSDevicePrepare(
                "{}",
                nullptr,
                [](void*) -> MaaBool { return true; },
                nullptr,
                result),
            "cancellation reaches control unit");
        MaaStringBufferDestroy(result);
        MaaStringBufferDestroy(credential);
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
