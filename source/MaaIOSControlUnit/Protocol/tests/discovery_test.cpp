#include "remote/Discovery.h"
#include "transport/Usbmux.h"
#include "wifi/Mdns.h"
#include "wifi/PairRecord.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
using namespace maa::ios;
using remote::DiscoveryPairing;
using remote::DiscoveryTransport;
int checks = 0, failures = 0;

void check(bool ok, const char* label)
{
    ++checks;
    if (!ok) {
        ++failures;
        std::printf("FAIL: %s\n", label);
    }
}

wifi::PairRecord paired_record()
{
    wifi::PairRecord record;
    record.udid = "00008110-TEST:DEVICE";
    record.host_identifier = "synthetic-host";
    record.peer_identifier = "synthetic-peer";
    record.host_private_key.assign(32, 1);
    record.host_public_key.assign(32, 2);
    record.peer_public_key.assign(32, 3);
    record.peer_alt_irk.resize(16);
    for (size_t i = 0; i < 16; ++i) {
        record.peer_alt_irk[i] = static_cast<uint8_t>(i);
    }
    record.advertised_identifier = "old-identifier";
    return record;
}

wifi::mdns::Advertisement advertisement()
{
    wifi::mdns::Advertisement ad;
    ad.instance = "synthetic._remotepairing._tcp.local.";
    // 已由 discovery_identity_test 的公开密钥与独立 OpenSSL CLI 对拍确定的标签。
    ad.identifier = "00000000-0000-0000-0000-000000000000";
    ad.auth_tag = "pdYyNGg1";
    ad.name = "Synthetic iPhone";
    ad.endpoints = { { "192.0.2.10", 49152, 4, "en4" }, { "fe80::1234%4", 49152, 4, "en4" }, { "fe80::1234%7", 49153, 7, "en7" } };
    return ad;
}

void test_merge()
{
    const auto record = paired_record();
    auto ad = advertisement();
    const std::vector<transport::DeviceRecord> usb = { { 10, record.udid, "USB", 1 },
                                                       { 11, record.udid, "Network", 1 },
                                                       { 12, "other-usb-device", "USB", 2 } };
    auto result = remote::detail::merge_discovery(usb, { ad, ad }, { record });
    check(result.devices.size() == 2, "unique authTag joins wireless candidates to the USB device");
    const auto& device = result.devices.front();
    check(device.udid == record.udid && device.name == ad.name, "real record UDID and optional broadcast name are preserved");
    check(device.candidates.size() == 5, "USB, usbmux Network and all wireless endpoints survive deduplication");
    check(
        device.candidates[0].transport == DiscoveryTransport::usbmux && device.candidates[0].connection_type == "USB"
            && device.candidates[0].device_id == 10 && device.candidates[1].connection_type == "Network"
            && device.candidates[1].device_id == 11,
        "usbmux connection kind and device_id remain separate connection candidates");
    check(
        device.candidates[2].pairing == DiscoveryPairing::ready && device.candidates[3].address == "fe80::1234%4"
            && device.candidates[3].interface_index == 4 && device.candidates[4].address == "fe80::1234%7"
            && device.candidates[4].interface_name == "en7" && device.candidates[4].port == 49153,
        "IPv6 scope, interface identity and real advertised ports are retained");

    auto second_instance = ad;
    second_instance.instance = "second._remotepairing._tcp.local.";
    result = remote::detail::merge_discovery({ }, { ad, second_instance }, { record });
    check(
        result.devices.size() == 1 && result.devices[0].candidates.size() == 6,
        "multiple tag-matched instances of a device retain all candidates");

    ad.endpoints.clear();
    result = remote::detail::merge_discovery({ }, { ad }, { record });
    check(
        result.devices.size() == 1 && result.devices[0].candidates.size() == 1 && result.devices[0].candidates[0].address.empty()
            && result.devices[0].candidates[0].port == 0,
        "partial PTR/TXT advertisements remain visible without fabricated endpoints");

    result = remote::detail::merge_discovery({ }, { advertisement() }, { });
    check(
        result.devices.size() == 1 && result.devices[0].udid.empty()
            && result.devices[0].candidates[0].pairing == DiscoveryPairing::unmatched,
        "an unknown wireless identifier is never presented as a USB UDID");
    auto first_unknown = advertisement(), second_unknown = first_unknown;
    second_unknown.endpoints = { { "192.0.2.11", 49154, 8, "en8" } };
    result = remote::detail::merge_discovery({ }, { first_unknown, second_unknown }, { });
    check(
        result.devices.size() == 1 && result.devices[0].candidates.size() == 4,
        "unknown instance and identifier merge across interfaces without losing addresses");
    auto conflicting_tag = first_unknown;
    conflicting_tag.auth_tag = "AAAAAAAA";
    result = remote::detail::merge_discovery({ }, { first_unknown, conflicting_tag }, { });
    check(
        result.devices.size() == 2 && result.devices[0].udid.empty() && result.devices[1].udid.empty(),
        "different authTags with equal unknown instance and identifier stay separate");
    check(
        result.devices[0].candidates.size() == first_unknown.endpoints.size()
            && result.devices[1].candidates.size() == conflicting_tag.endpoints.size()
            && result.devices[0].candidates == result.devices[1].candidates,
        "identical endpoints do not deduplicate conflicting unknown authTags across device rows");
    auto upper_identifier = first_unknown, lower_identifier = first_unknown;
    upper_identifier.identifier = "Opaque-Identifier";
    lower_identifier.identifier = "opaque-identifier";
    result = remote::detail::merge_discovery({ }, { upper_identifier, lower_identifier }, { });
    check(result.devices.size() == 2, "unknown identifiers retain original case-sensitive bytes");
    second_unknown.instance = "different._remotepairing._tcp.local.";
    result = remote::detail::merge_discovery({ }, { first_unknown, second_unknown }, { });
    check(result.devices.size() == 2, "equal untrusted identifiers do not merge different instances");
    second_unknown.instance = first_unknown.instance;
    second_unknown.identifier = "other-identifier";
    result = remote::detail::merge_discovery({ }, { first_unknown, second_unknown }, { });
    check(result.devices.size() == 2, "equal instance names do not merge different untrusted identifiers");

    auto legacy = record;
    legacy.advertised_identifier = first_unknown.identifier;
    legacy.peer_alt_irk.clear();
    result = remote::detail::merge_discovery(usb, { first_unknown }, { legacy });
    check(
        result.devices.size() == 3 && result.devices.back().udid.empty()
            && result.devices.back().candidates[0].pairing == DiscoveryPairing::identifier_hint,
        "legacy identifier equality is a hint and cannot select a record or merge with USB");
    legacy = record;
    legacy.peer_identifier.clear();
    legacy.peer_public_key.clear();
    result = remote::detail::merge_discovery(usb, { first_unknown }, { legacy });
    check(
        result.devices.size() == 2 && result.devices[0].candidates.back().pairing == DiscoveryPairing::needs_pairing,
        "unique tag with missing pinned identity identifies device but requires pairing");
    legacy = record;
    legacy.host_public_key.clear();
    result = remote::detail::merge_discovery({ }, { first_unknown }, { legacy });
    check(result.devices[0].candidates[0].pairing == DiscoveryPairing::needs_pairing, "missing host credentials do not appear ready");
    auto conflict = record;
    conflict.udid = "different-real-device";
    result = remote::detail::merge_discovery(usb, { first_unknown }, { record, conflict });
    check(
        result.devices.size() == 3 && result.devices.back().udid.empty()
            && result.devices.back().candidates[0].pairing == DiscoveryPairing::ambiguous,
        "multiple tag matches remain ambiguous and do not select first record");
    first_unknown.auth_tag = "broken";
    result = remote::detail::merge_discovery(usb, { first_unknown }, { record });
    check(
        result.devices.size() == 3 && result.devices.back().candidates[0].pairing == DiscoveryPairing::invalid_advertisement,
        "malformed tag remains a visible untrusted advertisement");
    result = remote::detail::merge_discovery({ }, { advertisement() }, { record });
    check(!result.usb_available && !result.wifi_available, "pure merging does not claim actual transport availability");
}

void test_non_network_paths()
{
    remote::DiscoveryOptions options;
    options.should_cancel = [] {
        return true;
    };
    options.pairing_records = { };
    auto result = remote::discover_devices(options);
    check(
        result.cancelled && result.devices.empty() && result.warnings.empty() && !result.usb_available && !result.wifi_available,
        "pre-cancelled discovery does not enumerate USB, scan Wi-Fi or read records");
    options.should_cancel = { };
    options.include_usb = false;
    options.timeout = std::chrono::milliseconds(0);
    result = remote::discover_devices(options);
    check(
        !result.cancelled && result.devices.empty() && result.warnings.empty() && !result.wifi_available,
        "zero timeout skips wireless network and record directory operations");
    options.timeout = std::chrono::milliseconds(-1);
    result = remote::discover_devices(options);
    check(
        result.devices.empty() && result.warnings.size() == 1 && !result.wifi_available,
        "negative timeout is rejected before networking");
    options.timeout = std::chrono::milliseconds(60001);
    result = remote::discover_devices(options);
    check(
        result.devices.empty() && result.warnings.size() == 1 && !result.wifi_available,
        "timeout above maximum is rejected before networking");
    options.timeout = std::chrono::milliseconds(3000);
    options.include_wifi = false;
    result = remote::discover_devices(options);
    check(
        result.devices.empty() && result.warnings.empty() && !result.usb_available && !result.wifi_available,
        "disabling both sources returns an empty snapshot without reading records");
}
} // namespace

int main()
{
    try {
        test_merge();
        test_non_network_paths();
    }
    catch (const std::exception& error) {
        ++failures;
        std::printf("FAIL: discovery fixture: %s\n", error.what());
    }
    std::printf("discovery: %d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
