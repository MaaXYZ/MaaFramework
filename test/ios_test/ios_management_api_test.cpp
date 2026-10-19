#include "MaaToolkit/IOSDevice/MaaToolkitIOSDevice.h"
#include "MaaUtils/Buffer/StringBuffer.hpp"
#include "MaaUtils/Runtime.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif
#include <cstdio>
#include <meojson/json.hpp>
#include <stdexcept>

namespace
{
bool backend_loaded()
{
#ifdef _WIN32
    return GetModuleHandleW(L"MaaIOSControlUnit.dll") != nullptr;
#else
#ifdef __APPLE__
    const auto name = "libMaaIOSControlUnit.dylib";
#else
    const auto name = "libMaaIOSControlUnit.so";
#endif
    auto* module = dlopen((MAA_NS::library_dir() / name).c_str(), RTLD_NOW | RTLD_NOLOAD);
    if (!module) {
        return false;
    }
    dlclose(module);
    return true;
#endif
}

int cancel_calls = 0;

MaaBool MAA_CALL cancelled(void*)
{
    ++cancel_calls;
    return true;
}

MaaBool MAA_CALL throwing_cancel(void*)
{
    throw std::runtime_error("caller failure");
}

bool result_is(const MAA_NS::StringBuffer& buffer, bool stopped)
{
    const auto result = json::parse(buffer.get());
    return result && result->get("success", true) == false && result->get("cancelled", !stopped) == stopped
           && !result->get("error", "").empty();
}
}

int main()
{
    if (backend_loaded()) {
        return 30;
    }
    MAA_NS::StringBuffer result;
    MaaIOSWiFiPairing* credential = nullptr;
    if (MaaToolkitIOSDeviceDiscover(MaaIOSConnectionMethod_Auto, nullptr, 0, "{}", nullptr, cancelled, nullptr, &result)
        || cancel_calls != 1 || !result_is(result, true)) {
        return 1;
    }
    if (!backend_loaded()) {
        return 31;
    }
    cancel_calls = 0;
    if (MaaToolkitIOSDeviceDiscover(MaaIOSConnectionMethod_Auto, nullptr, 0, "{\"timeout_ms\":-1}", nullptr, cancelled, nullptr, &result)
        || cancel_calls != 0 || !result_is(result, false)) {
        return 2;
    }
    if (MaaToolkitIOSDeviceCheckConnection(
            "synthetic-device",
            MaaIOSConnectionMethod_USB,
            nullptr,
            nullptr,
            "{\"wifi\":\"auto\",\"port\":49152}",
            nullptr,
            nullptr,
            nullptr,
            &result)
        || !result_is(result, false)) {
        return 3;
    }
    if (MaaToolkitIOSDevicePair(nullptr, MaaIOSConnectionMethod_WiFi, "{}", nullptr, nullptr, nullptr, &result, &credential)
        || !result_is(result, false) || credential != nullptr) {
        return 4;
    }
    if (MaaToolkitIOSDevicePrepare(
            "synthetic-device",
            MaaIOSConnectionMethod_Auto,
            nullptr,
            nullptr,
            "{\"replace\":1}",
            nullptr,
            nullptr,
            nullptr,
            &result)
        || !result_is(result, false)) {
        return 5;
    }
    if (MaaToolkitIOSDeviceDiscover(MaaIOSConnectionMethod_Auto, nullptr, 0, "{\"unexpected\":true}", nullptr, nullptr, nullptr, &result)
        || !result_is(result, false)) {
        return 6;
    }
    if (MaaToolkitIOSDeviceDiscover(MaaIOSConnectionMethod_Auto, nullptr, 0, "{}", nullptr, throwing_cancel, nullptr, &result)
        || !result_is(result, true)) {
        return 7;
    }
    if (MaaToolkitIOSDeviceDiscover(MaaIOSConnectionMethod_Auto, nullptr, 0, nullptr, nullptr, nullptr, nullptr, nullptr)) {
        return 8;
    }
    if (MaaToolkitIOSDeviceDiscover(MaaIOSConnectionMethod_Auto, nullptr, 0, "{", nullptr, nullptr, nullptr, &result)
        || !result_is(result, false)) {
        return 9;
    }
    const std::string record = "MaaIOS1|udid=synthetic-device|hid=synthetic-host|pid=peer|hsk=" + std::string(64, '1')
                               + "|hpk=" + std::string(64, '2') + "|ppk=" + std::string(64, '4');
    auto* pairing = MaaToolkitIOSDeviceParsePairingRecord(record.c_str());
    if (!pairing || std::string(pairing->udid) != "synthetic-device" || std::string(pairing->peer_identifier) != "peer"
        || pairing->host_private_key[0] != 0x11) {
        return 10;
    }
    if (!MaaToolkitIOSDeviceFormatPairingRecord(pairing, &result) || result.get() != record) {
        return 11;
    }
    auto view = *pairing;
    const std::string oversized_peer(4097, 'x');
    view.peer_identifier = oversized_peer.c_str();
    if (MaaToolkitIOSDeviceFormatPairingRecord(&view, &result) || !result.empty()) {
        return 15;
    }
    view = *pairing;
    view.peer_identifier = nullptr;
    if (MaaToolkitIOSDeviceFormatPairingRecord(&view, &result) || !result.empty()) {
        return 16;
    }
    view = *pairing;
    view.host_identifier = "invalid\nhost";
    if (MaaToolkitIOSDeviceFormatPairingRecord(&view, &result) || !result.empty()) {
        return 17;
    }
    view = *pairing;
    view.host_private_key[0] = 0x55;
    // All-zero IRKs are valid provided values, distinct from an absent IRK.
    view.has_peer_alt_irk = true;
    if (pairing->host_private_key[0] != 0x11 || !MaaToolkitIOSDeviceFormatPairingRecord(&view, &result)
        || result.get().find("irk=" + std::string(32, '0')) == std::string::npos) {
        return 18;
    }
    view.has_peer_alt_irk = false;
    if (!MaaToolkitIOSDeviceFormatPairingRecord(&view, &result) || result.get().find("irk=") != std::string::npos) {
        return 19;
    }
    const MaaIOSWiFiPairing* pairings[] = { pairing, pairing };
    cancel_calls = 0;
    if (MaaToolkitIOSDeviceDiscover(MaaIOSConnectionMethod_WiFi, pairings, 1, nullptr, nullptr, cancelled, nullptr, &result)
        || cancel_calls != 1 || !result_is(result, true)) {
        return 21;
    }
    cancel_calls = 0;
    if (MaaToolkitIOSDeviceDiscover(MaaIOSConnectionMethod_Auto, pairings, 2, nullptr, nullptr, cancelled, nullptr, &result)
        || cancel_calls != 0 || !result_is(result, false)) {
        return 22;
    }
    if (MaaToolkitIOSDevicePrepare(
            "wrong-device",
            MaaIOSConnectionMethod_WiFi,
            nullptr,
            pairing,
            nullptr,
            nullptr,
            cancelled,
            nullptr,
            &result)
        || cancel_calls != 0 || !result_is(result, false)) {
        return 23;
    }
    if (MaaToolkitIOSDeviceCheckConnection(nullptr, 99, nullptr, nullptr, nullptr, nullptr, cancelled, nullptr, &result)
        || cancel_calls != 0 || !result_is(result, false)) {
        return 24;
    }
    view.udid = nullptr;
    if (MaaToolkitIOSDevicePrepare(
            "synthetic-device",
            MaaIOSConnectionMethod_USB,
            "ignored",
            &view,
            nullptr,
            nullptr,
            cancelled,
            nullptr,
            &result)
        || cancel_calls != 1 || !result_is(result, true)) {
        return 25;
    }
    cancel_calls = 0;
    if (MaaToolkitIOSDeviceDiscover(MaaIOSConnectionMethod_Auto, nullptr, 1, nullptr, nullptr, cancelled, nullptr, &result)
        || cancel_calls != 0 || !result_is(result, false)) {
        return 26;
    }
    for (const auto connection : { MaaIOSConnectionMethod_Auto, MaaIOSConnectionMethod_USB, MaaIOSConnectionMethod_WiFi }) {
        for (const auto* udid : { static_cast<const char*>(nullptr), "" }) {
            cancel_calls = 0;
            if (MaaToolkitIOSDeviceCheckConnection(udid, connection, nullptr, pairing, nullptr, nullptr, cancelled, nullptr, &result)
                || cancel_calls != 0 || !result_is(result, false) || result.get().find("UDID is required") == std::string::npos) {
                return 27;
            }
            if (MaaToolkitIOSDevicePrepare(udid, connection, nullptr, pairing, nullptr, nullptr, cancelled, nullptr, &result)
                || cancel_calls != 0 || !result_is(result, false) || result.get().find("UDID is required") == std::string::npos) {
                return 28;
            }
        }
    }
    MaaToolkitIOSDeviceDestroyPairing(pairing);
    // Parse owns text independently from the serialized input.
    auto text = record;
    pairing = MaaToolkitIOSDeviceParsePairingRecord(text.c_str());
    text.assign(text.size(), 'x');
    if (!pairing || std::string(pairing->udid) != "synthetic-device") {
        return 12;
    }
    MaaToolkitIOSDeviceDestroyPairing(pairing);
    MaaToolkitIOSDeviceDestroyPairing(nullptr);
    if (MaaToolkitIOSDeviceParsePairingRecord("invalid") || MaaToolkitIOSDeviceParsePairingRecord(nullptr)) {
        return 13;
    }
    auto empty_peer_record = record;
    empty_peer_record.replace(empty_peer_record.find("pid=peer"), 8, "pid=");
    if (MaaToolkitIOSDeviceParsePairingRecord(empty_peer_record.c_str())) {
        return 20;
    }
    result.set("old value");
    if (MaaToolkitIOSDeviceFormatPairingRecord(nullptr, &result) || !result.empty()) {
        return 14;
    }
    cancel_calls = 0;
    if (MaaToolkitIOSDevicePair(
            "synthetic-device",
            MaaIOSConnectionMethod_USB,
            R"({"repair":true})",
            nullptr,
            cancelled,
            nullptr,
            &result,
            &credential)
        || credential || cancel_calls != 0 || !result_is(result, false)
        || result.get().find("unknown iOS management option: repair") == std::string::npos) {
        return 29;
    }
    std::puts("iOS management C boundary validation and cancellation passed");
    return 0;
}
