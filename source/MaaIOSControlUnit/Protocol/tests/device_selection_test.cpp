#include "remote/Device.h"
#include "remote/DeviceConnection.h"

#include <array>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>
#ifndef _WIN32
#include <cstdlib>
#include <sys/un.h>
#include <unistd.h>
#endif

namespace
{
using namespace maa::ios;
using namespace std::chrono_literals;
int checks = 0, failures = 0;

void check(bool ok, const char* label)
{
    ++checks;
    if (!ok) {
        ++failures;
        std::printf("FAIL: %s\n", label);
    }
}

wifi::PairRecord record(std::string udid = "device:one-original")
{
    wifi::PairRecord paired;
    paired.udid = std::move(udid);
    paired.host_identifier = "synthetic-host";
    paired.host_private_key.assign(32, 1);
    paired.host_public_key.assign(32, 2);
    paired.peer_identifier = wifi::bytes_of("synthetic-peer");
    paired.peer_public_key.assign(32, 3);
    return paired;
}

remote::DiscoveryCandidate candidate(std::string address, uint16_t port, uint32_t index = 0, std::string interface = { })
{
    remote::DiscoveryCandidate value;
    value.transport = remote::DiscoveryTransport::remote_pairing;
    value.connection_type = "RemotePairing";
    value.address = std::move(address);
    value.port = port;
    value.interface_index = index;
    value.interface_name = std::move(interface);
    value.instance = "synthetic._remotepairing._tcp.local.";
    value.identifier = "synthetic-advertisement";
    value.pairing = remote::DiscoveryPairing::ready;
    return value;
}

void test_record_selection()
{
    std::string error;
    const auto paired = record();
    auto legacy = record("legacy-device");
    legacy.peer_public_key.clear();
    auto incomplete = record("incomplete-device");
    incomplete.host_private_key.clear();
    auto selected = remote::detail::select_pairing_record({ legacy, incomplete, paired }, { }, error);
    check(selected && selected->udid == paired.udid && error.empty(), "manual Wi-Fi chooses the only complete record with pinned identity");
    selected = remote::detail::select_pairing_record({ paired }, "device_one_original", error);
    check(!selected && !error.empty(), "sanitized filename text cannot stand in for original record UDID");
    selected = remote::detail::select_pairing_record({ paired, record("second") }, paired.udid, error);
    check(selected && selected->udid == paired.udid && error.empty(), "manual serial selects exact original record UDID");
    check(
        !remote::detail::select_pairing_record({ paired, record("second") }, { }, error),
        "multiple complete records require explicit selection");
    check(
        !remote::detail::select_pairing_record({ paired, paired }, paired.udid, error),
        "duplicate records with one UDID remain ambiguous rather than selecting first");
    check(
        !remote::detail::select_pairing_record({ legacy, incomplete }, { }, error),
        "records lacking host credentials or device identity are unusable for Wi-Fi");
}

void test_discovered_selection()
{
    std::string error;
    remote::DiscoveredDevice device;
    device.udid = record().udid;
    device.candidates = { candidate("fe80::1234%7", 49159, 7, "en7"),
                          candidate("169.254.2.3", 49156, 7, "en7"),
                          candidate("2001:db8::1", 49157, 4, "en4"),
                          candidate("192.0.2.4", 49154, 4, "en4"),
                          candidate("192.0.2.1", 49151, 1, "Wi-Fi") };
    auto selection = remote::detail::select_paired_wireless_device({ device }, { }, error);
    check(
        selection && selection->udid == device.udid && selection->candidates.size() == 5,
        "single paired discovered device retains all endpoints");
    check(
        selection->candidates[0].address == "192.0.2.1" && selection->candidates[1].address == "192.0.2.4"
            && selection->candidates[2].address == "2001:db8::1" && selection->candidates[3].address == "169.254.2.3"
            && selection->candidates[4].address == "fe80::1234%7",
        "ordinary IPv4 with Wi-Fi preference precedes IPv6 and link-local endpoints");
    check(
        selection->candidates[0].port == 49151 && selection->candidates[4].port == 49159 && selection->candidates[4].interface_index == 7,
        "discovery preserves each SRV port and scoped interface identity");

    auto other = device;
    other.udid = "second-device";
    check(
        !remote::detail::select_paired_wireless_device({ device, other }, { }, error),
        "multiple paired devices cannot be resolved by picking first");
    selection = remote::detail::select_paired_wireless_device({ device, other }, device.udid, error);
    check(selection && selection->udid == device.udid, "auto Wi-Fi accepts exact original serial filter");
    check(
        !remote::detail::select_paired_wireless_device({ device }, "synthetic-advertisement", error),
        "advertised identifier is not a device serial filter");
    check(
        !remote::detail::select_paired_wireless_device({ device }, "device_one_original", error),
        "sanitized record filename is not an auto Wi-Fi serial");

    auto untrusted = device;
    untrusted.udid.clear();
    check(!remote::detail::select_paired_wireless_device({ untrusted }, { }, error), "unknown untrusted UDID cannot connect automatically");
    for (const auto state : { remote::DiscoveryPairing::unmatched,
                              remote::DiscoveryPairing::identifier_hint,
                              remote::DiscoveryPairing::needs_pairing,
                              remote::DiscoveryPairing::ambiguous,
                              remote::DiscoveryPairing::invalid_advertisement,
                              remote::DiscoveryPairing::crypto_error }) {
        untrusted = device;
        for (auto& value : untrusted.candidates) {
            value.pairing = state;
        }
        check(
            !remote::detail::select_paired_wireless_device({ untrusted }, { }, error),
            "only unique ready authTag match is an automatic connection candidate");
    }
    device.candidates = { candidate("", 49152), candidate("192.0.2.1", 0) };
    check(!remote::detail::select_paired_wireless_device({ device }, { }, error), "incomplete address or SRV port cannot be fabricated");
    device.candidates = { candidate("fe80::1", 49152, 9, "en9"),
                          candidate("FEA0::2", 49153, 10, "en10"),
                          candidate("fe80::3%en4", 49154, 4, "en4") };
    selection = remote::detail::select_paired_wireless_device({ device }, { }, error);
    check(
        selection && selection->candidates[0].address == "fe80::1%9" && selection->candidates[1].address == "FEA0::2%10"
            && selection->candidates[2].address == "fe80::3%en4",
        "missing numeric IPv6 scopes are restored and existing named scope remains unchanged");
    auto extra_instance = device;
    for (auto& value : extra_instance.candidates) {
        value.instance = "other-instance";
    }
    selection = remote::detail::select_paired_wireless_device({ device, extra_instance, device }, { }, error);
    check(
        selection && selection->candidates.size() == 6,
        "same real device across instances stays one choice and exact duplicates are removed");
}

void test_connection_attempts()
{
    const auto paired = record();
    remote::detail::WirelessSelection selection {
        paired.udid,
        { candidate("192.0.2.1", 50001), candidate("192.0.2.2", 50002), candidate("fe80::1%9", 50003, 9) }
    };
    std::vector<std::pair<std::string, uint16_t>> attempts;
    std::string error;
    auto connect =
        [&](const std::string& address, const wifi::PairRecord& used, uint16_t port, std::string& err) -> std::optional<remote::Device> {
        check(used.udid == paired.udid, "every address uses the selected pinned pairing record");
        attempts.emplace_back(address, port);
        if (attempts.size() == 2) {
            return remote::Device { };
        }
        err = "synthetic first failure";
        return std::nullopt;
    };
    auto device = remote::detail::connect_wireless_candidates(selection, paired, error, connect);
    check(
        device && attempts == std::vector<std::pair<std::string, uint16_t>> { { "192.0.2.1", 50001 }, { "192.0.2.2", 50002 } }
            && error.empty(),
        "first endpoint failure tries next SRV endpoint and success stops remaining attempts");
    attempts.clear();
    auto fail_all =
        [&](const std::string& address, const wifi::PairRecord&, uint16_t port, std::string& err) -> std::optional<remote::Device> {
        attempts.emplace_back(address, port);
        err = "failure-" + std::to_string(port);
        return std::nullopt;
    };
    device = remote::detail::connect_wireless_candidates(selection, paired, error, fail_all);
    check(
        !device && attempts.size() == 3 && attempts.back() == std::pair<std::string, uint16_t> { "fe80::1%9", 50003 },
        "all endpoints are attempted including scoped IPv6 when earlier addresses fail");
    check(
        error.find("failure-50001") != std::string::npos && error.find("failure-50002") != std::string::npos
            && error.find("failure-50003") != std::string::npos,
        "all failed attempts retain their errors for diagnosis");
    attempts.clear();
    device = remote::detail::connect_wireless_candidates(selection, record("other-device"), error, fail_all);
    check(!device && attempts.empty(), "mismatched internal UDID fails before any network attempt");
    auto legacy = paired;
    legacy.peer_identifier.clear();
    device = remote::detail::connect_wireless_candidates(selection, legacy, error, fail_all);
    check(!device && attempts.empty(), "missing peer identity fails before connection");
}

void test_usbmux_precedence()
{
    const std::vector<transport::DeviceRecord> devices { { 1, "usb-device", "USB", 0 }, { 2, "mux-network", "Network", 0 } };
    check(remote::detail::has_usbmux_match(devices, { }), "any existing usbmux device selects local connection before discovery");
    check(remote::detail::has_usbmux_match(devices, "usb-device"), "matching USB serial selects USB protocol path");
    check(remote::detail::has_usbmux_match(devices, "mux-network"), "existing usbmux Network entry retains its original transport path");
    check(
        !remote::detail::has_usbmux_match(devices, "wireless-only"),
        "absent serial allows discovery instead of selecting unrelated USB device");
    check(!remote::detail::has_usbmux_match({ }, { }), "empty usbmux list allows wireless discovery");
}

void test_usbmux_device_selection()
{
    const std::string first = "00008110-PRIVATE-FIRST-AAAA";
    const std::string second = "00008110-PRIVATE-SECOND-BBBB";
    const std::string absent = "00008110-PRIVATE-ABSENT-CCCC";
    const transport::DeviceRecord usb { 10, first, "USB", 1 };
    const transport::DeviceRecord network { 11, first, "Network", 1 };
    const transport::DeviceRecord other { 12, second, "USB", 2 };
    std::string error = "stale error";
    auto select = [&](const std::vector<transport::DeviceRecord>& records, std::string_view serial = { }, bool usb_only = false) {
        return remote::detail::select_usbmux_device(records, serial, error, usb_only);
    };
    auto selected = select({ network, usb });
    check(
        selected && selected->device_id == usb.device_id && selected->connection_type == "USB" && error.empty(),
        "same UDID USB and Network entries are one device and USB is preferred");
    selected = select({ usb, network });
    check(selected && selected->device_id == usb.device_id, "USB preference is independent of usbmux list ordering");
    selected = select({ network, other, usb }, first);
    check(
        selected && selected->device_id == usb.device_id && selected->udid == first && error.empty(),
        "explicit original serial prefers USB without selecting a different device");
    check(
        !select({ network, usb, other }) && error.find("Connected devices: 2") != std::string::npos,
        "automatic selection counts distinct UDIDs rather than connection records");
    check(
        error.find(first) == std::string::npos && error.find(second) == std::string::npos
            && error.find(remote::mask(first)) != std::string::npos && error.find(remote::mask(second)) != std::string::npos,
        "multiple-device error only displays masked UDIDs");
    check(
        !select({ network, usb, other }, absent) && error.find(absent) == std::string::npos && error.find(first) == std::string::npos
            && error.find(second) == std::string::npos && error.find(remote::mask(absent)) != std::string::npos,
        "unmatched serial and candidates are masked in the selection error");
    selected = select({ network });
    check(
        selected && selected->device_id == network.device_id && error.empty(),
        "unique usbmux Network connection remains usable when USB is absent");
    auto duplicate_usb = usb;
    duplicate_usb.device_id = 13;
    check(
        !select({ usb, duplicate_usb, network }) && !error.empty() && error.find(first) == std::string::npos,
        "multiple USB connections for one UDID are rejected rather than choosing first or falling back");
    check(!select({ network, duplicate_usb, usb }, first), "explicit serial cannot hide duplicate USB connections");
    auto duplicate_network = network;
    duplicate_network.device_id = 14;
    check(!select({ network, duplicate_network }), "multiple Network connections for one UDID are rejected without USB");
    selected = select({ network, duplicate_network, usb });
    check(
        selected && selected->device_id == usb.device_id,
        "duplicate lower-priority Network entries do not prevent choosing the unique USB connection");
    selected = select({ network, usb }, { }, true);
    check(selected && selected->device_id == usb.device_id, "USB-only mode filters Network before grouping");
    check(!select({ network }, first, true), "USB-only mode never selects a Network connection");
    check(!select({ usb, duplicate_usb }, first, true), "USB-only mode rejects duplicate USB connections");
    check(!select({ }), "empty list cannot select a device");
    const transport::DeviceRecord empty_usb { 20, "", "USB", 0 };
    const transport::DeviceRecord empty_network { 21, "", "Network", 0 };
    check(!select({ empty_usb }) && !error.empty(), "empty UDID is not a unique real USB device");
    check(!select({ empty_network }) && !error.empty(), "empty UDID is not a unique real Network device");
    check(!select({ empty_usb, empty_network }, { }, true), "USB-only filtering cannot make an empty UDID usable");
    selected = select({ empty_usb, usb });
    check(
        selected && selected->device_id == usb.device_id && selected->udid == first,
        "unidentifiable entries cannot replace a uniquely identified device");
    selected = select({ empty_usb, other, network, usb }, second, true);
    check(
        selected && selected->device_id == other.device_id,
        "explicit USB-only selection preserves exact serial among other devices and empty IDs");
}

void test_default_connection_policy()
{
    using transport::UsbmuxDiscoveryStatus;
    auto status = UsbmuxDiscoveryStatus::complete;
    std::vector<transport::DeviceRecord> records { { 1, "first", "USB", 0 } };
    int enumerations = 0, usb_attempts = 0, wifi_attempts = 0;
    bool cancel = false, fail_usb = false, fail_wifi = false, cancel_after_enumeration = false;
    std::string error, selected_serial, enumeration_error;
    std::vector<std::string> warnings;
    remote::detail::DefaultDeviceOperations operations {
        [&](auto& out, std::chrono::milliseconds budget, const auto& should_cancel, std::string& err) {
            ++enumerations;
            check(budget == 1000ms && !should_cancel(), "default preselection has a bounded one-second budget and live cancel callback");
            out = records;
            err = enumeration_error;
            if (cancel_after_enumeration) {
                cancel = true;
            }
            return status;
        },
        [&](const std::string& serial, std::string& err) -> std::optional<remote::Device> {
            ++usb_attempts;
            selected_serial = serial;
            if (fail_usb) {
                err = "synthetic USB protocol failure";
                return std::nullopt;
            }
            return remote::Device { };
        },
        [&](const std::string& serial, std::string& err) -> std::optional<remote::Device> {
            ++wifi_attempts;
            selected_serial = serial;
            if (fail_wifi) {
                err = "synthetic wireless failure";
                return std::nullopt;
            }
            return remote::Device { };
        },
        [&](const std::string& warning) { warnings.push_back(warning); },
    };
    auto open = [&](std::string serial = { }) {
        enumerations = usb_attempts = wifi_attempts = 0;
        warnings.clear();
        error = "stale error";
        return remote::detail::open_default_device(serial, error, [&] { return cancel; }, operations);
    };
    auto device = open();
    check(
        device && enumerations == 1 && usb_attempts == 1 && wifi_attempts == 0 && selected_serial.empty() && error.empty()
            && warnings.empty(),
        "complete unique USB snapshot preserves default establish serial and bypasses wireless");
    records.push_back({ 2, "second", "USB", 0 });
    device = open();
    check(
        !device && !error.empty() && usb_attempts == 0 && wifi_attempts == 0,
        "complete multiple-device snapshot fails before any USB or wireless connection");
    device = open("first");
    check(
        device && selected_serial == "first" && usb_attempts == 1 && wifi_attempts == 0,
        "complete list honors exact serial among other connected devices");
    records = { { 1, "first", "USB", 0 }, { 2, "first", "USB", 0 } };
    device = open("first");
    check(!device && usb_attempts == 0 && wifi_attempts == 0, "duplicate same-transport connections are not hidden by preselection");
    records = { { 1, "first", "USB", 0 }, { 2, "first", "Network", 0 } };
    device = open();
    check(device && usb_attempts == 1 && wifi_attempts == 0, "same UDID USB and Network entries keep existing USB selection policy");
    fail_usb = true;
    device = open();
    check(
        !device && error == "synthetic USB protocol failure" && wifi_attempts == 0,
        "USB authentication or protocol failure is returned without wireless fallback");
    fail_usb = false;
    device = open("wireless-only");
    check(
        device && selected_serial == "wireless-only" && usb_attempts == 0 && wifi_attempts == 1 && warnings.empty(),
        "confirmed nonmatching USB snapshot falls back with the requested original serial");
    records.clear();
    device = open();
    check(
        device && usb_attempts == 0 && wifi_attempts == 1 && warnings.empty(),
        "confirmed empty USB snapshot falls back without an unavailable warning");
    // Even a partial list from a failed source must not resolve device ambiguity.
    records = { { 1, "first", "USB", 0 }, { 2, "second", "USB", 0 } };
    status = UsbmuxDiscoveryStatus::timed_out;
    device = open();
    check(
        device && usb_attempts == 0 && wifi_attempts == 1 && warnings.size() == 1 && error.empty(),
        "timed-out USB source retains wireless fallback and warns even when fallback succeeds");
    status = UsbmuxDiscoveryStatus::unavailable;
    enumeration_error = "synthetic daemon unavailable";
    device = open();
    check(
        device && usb_attempts == 0 && wifi_attempts == 1 && warnings.size() == 1
            && warnings[0].find(enumeration_error) != std::string::npos && error.empty(),
        "unavailable source reports its diagnostic even when wireless fallback succeeds");
    fail_wifi = true;
    device = open();
    check(
        !device && usb_attempts == 0 && wifi_attempts == 1 && warnings.size() == 1
            && error.find("synthetic wireless failure") != std::string::npos && error.find(enumeration_error) != std::string::npos,
        "unavailable source and failed wireless fallback retain both diagnostics");
    fail_wifi = false;
    status = UsbmuxDiscoveryStatus::cancelled;
    device = open();
    check(
        !device && usb_attempts == 0 && wifi_attempts == 0 && warnings.empty() && error == "Device startup cancelled",
        "enumerator cancellation is not treated as an unavailable USB source");
    status = UsbmuxDiscoveryStatus::complete;
    cancel_after_enumeration = true;
    device = open();
    check(
        !device && usb_attempts == 0 && wifi_attempts == 0 && error == "Device startup cancelled",
        "caller cancellation after a complete snapshot prevents all connection attempts");
    cancel_after_enumeration = false;
    device = open();
    check(
        !device && enumerations == 0 && usb_attempts == 0 && wifi_attempts == 0,
        "pre-cancelled startup does not access USB or wireless sources");
    for (const std::string wifi : { "", "auto", "192.0.2.1" }) {
        device = remote::open_device({ }, wifi, error, 49152, [] { return true; });
        check(!device && error == "Device startup cancelled", "public default, auto and manual connection paths honor pre-cancellation");
    }

    remote::detail::WirelessSelection wireless { "first", { candidate("192.0.2.1", 49152), candidate("192.0.2.2", 49153) } };
    cancel = false;
    int candidates = 0;
    device = remote::detail::connect_wireless_candidates(
        wireless,
        record("first"),
        error,
        [&](const auto&, const auto&, auto, std::string& err) -> std::optional<remote::Device> {
            ++candidates;
            err = "synthetic first failure";
            cancel = true;
            return std::nullopt;
        },
        [&] { return cancel; });
    check(
        !device && candidates == 1 && error == "Device startup cancelled",
        "cancellation between wireless candidates prevents starting another authentication");
}

uint32_t read_le(const uint8_t* data)
{
    return uint32_t(data[0]) | uint32_t(data[1]) << 8 | uint32_t(data[2]) << 16 | uint32_t(data[3]) << 24;
}

void write_le(uint8_t* data, uint32_t value)
{
    for (unsigned index = 0; index < 4; ++index) {
        data[index] = static_cast<uint8_t>(value >> (index * 8));
    }
}

// Private fake daemon: actual connect/accept and framed ListDevices, never the system service.
class SelectionServer
{
public:
    transport::detail::UsbmuxDiscoveryEndpoint endpoint;
    std::atomic<bool> valid_request { false };

    explicit SelectionServer(std::optional<std::vector<transport::DeviceRecord>> response)
    {
        std::string error;
        if (!transport::initialize_sockets(error)) {
            throw std::runtime_error(error);
        }
#ifdef _WIN32
        listener_.reset(::socket(AF_INET, SOCK_STREAM, 0));
        sockaddr_in address { };
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
#else
        char directory[] = "/tmp/maa-ios-select-XXXXXX";
        if (!::mkdtemp(directory)) {
            throw std::runtime_error("mkdtemp failed");
        }
        directory_ = directory;
        endpoint.path = directory_ + "/socket";
        listener_.reset(::socket(AF_UNIX, SOCK_STREAM, 0));
        sockaddr_un address { };
        address.sun_family = AF_UNIX;
        std::memcpy(address.sun_path, endpoint.path.c_str(), endpoint.path.size() + 1);
#endif
        if (!listener_.valid() || ::bind(listener_.fd(), reinterpret_cast<sockaddr*>(&address), sizeof(address))) {
            throw std::runtime_error("private bind failed");
        }
#ifdef _WIN32
        socklen_t size = sizeof(address);
        if (::getsockname(listener_.fd(), reinterpret_cast<sockaddr*>(&address), &size)) {
            throw std::runtime_error("getsockname failed");
        }
        endpoint.port = ntohs(address.sin_port);
#endif
        if (::listen(listener_.fd(), 1)) {
            throw std::runtime_error("private listen failed");
        }
        worker_ = std::thread([this, response = std::move(response)] {
            while (!stopped_ && transport::wait_socket(listener_.fd(), false, 10) <= 0) {
            }
            if (stopped_) {
                return;
            }
            transport::Socket peer(::accept(listener_.fd(), nullptr, nullptr));
            std::string error;
            peer.set_read_timeout(500, error);
            std::array<uint8_t, 16> header { };
            if (!peer.read_exact(header.data(), header.size(), error)) {
                return;
            }
            const auto total = read_le(header.data());
            if (total < 16 || total > 4096 || read_le(header.data() + 4) != 1 || read_le(header.data() + 8) != 8
                || read_le(header.data() + 12) != 1) {
                return;
            }
            std::string body(total - 16, '\0');
            if (!peer.read_exact(body.data(), body.size(), error)) {
                return;
            }
            while (!body.empty() && body.back() == '\0') {
                body.pop_back();
            }
            const auto request = plist::parse(body);
            const auto type = request ? request->find("MessageType") : nullptr;
            if (!type || type->as_string_or() != "ListDevices") {
                return;
            }
            valid_request = true;
            if (response) {
                auto list = plist::Value::Array();
                for (const auto& device : *response) {
                    auto properties = plist::Value::Dict();
                    properties.set("SerialNumber", plist::Value::Str(device.udid));
                    properties.set("ConnectionType", plist::Value::Str(device.connection_type));
                    auto entry = plist::Value::Dict();
                    entry.set("DeviceID", plist::Value::Int(device.device_id));
                    entry.set("Properties", std::move(properties));
                    list.push(std::move(entry));
                }
                auto reply = plist::Value::Dict();
                reply.set("DeviceList", std::move(list));
                auto xml = plist::write(reply);
                std::vector<uint8_t> packet(16 + xml.size() + 1);
                write_le(packet.data(), static_cast<uint32_t>(packet.size()));
                write_le(packet.data() + 4, 1);
                write_le(packet.data() + 8, 8);
                write_le(packet.data() + 12, 1);
                std::memcpy(packet.data() + 16, xml.data(), xml.size());
                if (!peer.write_all(packet.data(), packet.size(), error)) {
                    return;
                }
            }
            while (!stopped_) {
                std::this_thread::sleep_for(1ms);
            }
        });
    }

    ~SelectionServer()
    {
        stopped_ = true;
        if (worker_.joinable()) {
            worker_.join();
        }
        listener_.close();
#ifndef _WIN32
        ::unlink(endpoint.path.c_str());
        ::rmdir(directory_.c_str());
#endif
    }

private:
    transport::Socket listener_;
    std::atomic<bool> stopped_ { false };
    std::thread worker_;
#ifndef _WIN32
    std::string directory_;
#endif
};

void test_default_private_daemon()
{
    auto run = [](std::optional<std::vector<transport::DeviceRecord>> reply, bool cancel) {
        const bool stalled = !reply.has_value();
        const bool ambiguous = reply && reply->size() > 1;
        SelectionServer server(std::move(reply));
        int usb_attempts = 0, wifi_attempts = 0, cancel_checks = 0;
        auto observed_status = transport::UsbmuxDiscoveryStatus::unavailable;
        std::vector<std::string> warnings;
        std::string error;
        const auto start = std::chrono::steady_clock::now();
        auto should_cancel = [&] {
            ++cancel_checks;
            return cancel && server.valid_request && std::chrono::steady_clock::now() - start > 80ms;
        };
        auto device = remote::detail::open_default_device(
            { },
            error,
            should_cancel,
            {
                [&](auto& out, auto budget, const auto& stop, auto& err) {
                    observed_status = transport::detail::list_usbmux_devices_at(server.endpoint, out, budget, stop, err);
                    return observed_status;
                },
                [&](const auto&, auto&) -> std::optional<remote::Device> {
                    ++usb_attempts;
                    return remote::Device { };
                },
                [&](const auto&, auto&) -> std::optional<remote::Device> {
                    ++wifi_attempts;
                    return remote::Device { };
                },
                [&](const auto& warning) { warnings.push_back(warning); },
            });
        const auto elapsed = std::chrono::steady_clock::now() - start;
        std::printf(
            "default_selection_private: case=%s status=%d elapsed_ms=%lld usb=%d wifi=%d warnings=%zu\n",
            cancel      ? "cancel"
            : stalled   ? "stalled"
            : ambiguous ? "ambiguous"
                        : "empty",
            static_cast<int>(observed_status),
            static_cast<long long>(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count()),
            usb_attempts,
            wifi_attempts,
            warnings.size());
        check(server.valid_request, "default selection sends real framed ListDevices to its private fake daemon");
        if (cancel) {
            check(
                !device && usb_attempts == 0 && wifi_attempts == 0 && warnings.empty() && error == "Device startup cancelled"
                    && elapsed < 500ms && cancel_checks > 2 && observed_status == transport::UsbmuxDiscoveryStatus::cancelled,
                "private daemon stalled reply is cancellable without starting wireless fallback");
        }
        else if (stalled) {
            check(
                device && usb_attempts == 0 && wifi_attempts == 1 && error.empty() && warnings.size() == 1 && elapsed >= 900ms
                    && elapsed < 3s && observed_status == transport::UsbmuxDiscoveryStatus::timed_out,
                "private daemon stalled reply reaches warned wireless fallback within the shared one-second budget");
        }
        else if (!ambiguous) {
            check(
                device && usb_attempts == 0 && wifi_attempts == 1 && warnings.empty() && error.empty()
                    && observed_status == transport::UsbmuxDiscoveryStatus::complete,
                "real confirmed-empty reply permits wireless fallback without source warning");
        }
        else {
            check(
                !device && usb_attempts == 0 && wifi_attempts == 0 && warnings.empty() && !error.empty()
                    && observed_status == transport::UsbmuxDiscoveryStatus::complete,
                "real complete ambiguous reply rejects selection rather than trying a wireless device");
        }
    };
    run(std::nullopt, false);
    run(std::nullopt, true);
    run(std::vector<transport::DeviceRecord> { }, false);
    run(std::vector<transport::DeviceRecord> { { 1, "first", "USB", 0 }, { 2, "second", "USB", 0 } }, false);
}
} // namespace

int main()
{
    test_record_selection();
    test_discovered_selection();
    test_connection_attempts();
    test_usbmux_precedence();
    test_usbmux_device_selection();
    test_default_connection_policy();
    test_default_private_daemon();
    std::printf("device_selection: %d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
