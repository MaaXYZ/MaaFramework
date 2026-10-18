#include "remote/DeviceConnection.h"

#include "MaaUtils/Logger.h"
#include "wifi/PairRecord.h"

#include <algorithm>
#include <cctype>
#include <map>
#include <tuple>

namespace maa::ios::remote
{
namespace
{
bool startup_cancelled(const std::function<bool()>& should_cancel, std::string& error)
{
    if (!should_cancel || !should_cancel()) {
        return false;
    }
    error = "Device startup cancelled";
    return true;
}

bool ipv6_link_local(std::string_view address)
{
    // fe80::/10，包含 fe80..febf。地址可带数字或接口名 scope。
    if (address.size() < 4) {
        return false;
    }
    auto lower = [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    };
    return lower(address[0]) == 'f' && lower(address[1]) == 'e'
           && (address[2] == '8' || address[2] == '9' || lower(address[2]) == 'a' || lower(address[2]) == 'b');
}

int address_priority(const remote::DiscoveryCandidate& candidate)
{
    if (candidate.address.find(':') == std::string::npos) {
        return candidate.address.starts_with("169.254.") ? 2 : 0;
    }
    return ipv6_link_local(candidate.address) ? 3 : 1;
}

bool wifi_interface(std::string name)
{
    std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return std::tolower(c); });
    return name.find("wifi") != std::string::npos || name.find("wi-fi") != std::string::npos || name.find("wireless") != std::string::npos
           || name.starts_with("wlan") || name == "en0";
}

void preserve_scope(remote::DiscoveryCandidate& candidate)
{
    if (!ipv6_link_local(candidate.address) || candidate.address.find('%') != std::string::npos) {
        return;
    }
    if (candidate.interface_index) {
        candidate.address += "%" + std::to_string(candidate.interface_index);
    }
    else if (!candidate.interface_name.empty()) {
        candidate.address += "%" + candidate.interface_name;
    }
}

std::optional<wifi::PairRecord> load_selected_record(
    std::string_view serial,
    std::string& error,
    const std::function<bool()>& should_cancel,
    const std::vector<wifi::PairRecord>& records)
{
    if (startup_cancelled(should_cancel, error)) {
        return std::nullopt;
    }
    return detail::select_pairing_record(records, serial, error);
}

std::optional<remote::Device> open_discovered_device(
    const std::string& serial,
    std::string& error,
    const std::function<bool()>& should_cancel,
    const transport::IoBudget& budget,
    const std::vector<wifi::PairRecord>& records)
{
    remote::DiscoveryOptions options;
    options.include_usb = false;
    options.pairing_records = records;
    options.timeout = std::min(options.timeout, budget.remaining());
    options.should_cancel = should_cancel;
    const auto discovery = remote::discover_devices(options);
    if (discovery.cancelled || startup_cancelled(should_cancel, error)) {
        error = "Device startup cancelled";
        return std::nullopt;
    }
    auto selection = detail::select_paired_wireless_device(discovery.devices, serial, error);
    if (!selection) {
        for (const auto& warning : discovery.warnings) {
            error += "\n" + warning;
        }
        return std::nullopt;
    }
    const auto record = load_selected_record(selection->udid, error, should_cancel, records);
    if (!record) {
        return std::nullopt;
    }
    return detail::connect_wireless_candidates(
        *selection,
        *record,
        error,
        [&budget](const std::string& address, const wifi::PairRecord& paired, uint16_t port, std::string& err) {
            return remote::Device::establish_wifi_bounded(address, paired, budget, err, false, port);
        },
        should_cancel);
}
} // namespace

bool detail::has_usbmux_match(const std::vector<transport::DeviceRecord>& devices, std::string_view serial)
{
    return std::any_of(devices.begin(), devices.end(), [&](const auto& device) { return serial.empty() || device.udid == serial; });
}

std::optional<wifi::PairRecord>
    detail::select_pairing_record(const std::vector<wifi::PairRecord>& records, std::string_view serial, std::string& error)
{
    error.clear();
    const wifi::PairRecord* selected = nullptr;
    for (const auto& record : records) {
        if (!record.complete() || !record.has_peer_identity() || (!serial.empty() && record.udid != serial)) {
            continue;
        }
        if (selected) {
            error = "Multiple usable remote pairing records match; select a unique device with -s";
            return std::nullopt;
        }
        selected = &record;
    }
    if (!selected) {
        error = "No usable remote pairing record matches; pair the device over USB first";
        return std::nullopt;
    }
    return *selected;
}

std::optional<detail::WirelessSelection>
    detail::select_paired_wireless_device(const std::vector<remote::DiscoveredDevice>& devices, std::string_view serial, std::string& error)
{
    error.clear();
    std::map<std::string, WirelessSelection> choices;
    for (const auto& device : devices) {
        if (device.udid.empty() || (!serial.empty() && device.udid != serial)) {
            continue;
        }
        for (auto candidate : device.candidates) {
            if (candidate.transport != remote::DiscoveryTransport::remote_pairing || candidate.pairing != remote::DiscoveryPairing::ready
                || candidate.address.empty() || candidate.port == 0) {
                continue;
            }
            preserve_scope(candidate);
            auto& choice = choices[device.udid];
            choice.udid = device.udid;
            if (std::find(choice.candidates.begin(), choice.candidates.end(), candidate) == choice.candidates.end()) {
                choice.candidates.push_back(std::move(candidate));
            }
        }
    }
    if (choices.empty()) {
        error = "No paired wireless device is available; check the network and pair over USB first";
        return std::nullopt;
    }
    if (choices.size() > 1) {
        error = "Multiple paired wireless devices are available; select one with -s";
        return std::nullopt;
    }
    auto selected = std::move(choices.begin()->second);
    std::stable_sort(selected.candidates.begin(), selected.candidates.end(), [](const auto& left, const auto& right) {
        return std::tuple(address_priority(left), !wifi_interface(left.interface_name))
               < std::tuple(address_priority(right), !wifi_interface(right.interface_name));
    });
    return selected;
}

std::optional<remote::Device> detail::connect_wireless_candidates(
    const WirelessSelection& selection,
    const wifi::PairRecord& record,
    std::string& error,
    const WifiConnector& connect,
    const std::function<bool()>& should_cancel)
{
    error.clear();
    if (startup_cancelled(should_cancel, error)) {
        return std::nullopt;
    }
    if (selection.udid.empty() || record.udid != selection.udid || !record.complete() || !record.has_peer_identity()) {
        error = "Selected wireless device does not match a usable pairing record";
        return std::nullopt;
    }
    if (!connect || selection.candidates.empty()) {
        error = "No usable wireless connection candidate is available";
        return std::nullopt;
    }
    std::string failures;
    for (const auto& candidate : selection.candidates) {
        if (startup_cancelled(should_cancel, error)) {
            return std::nullopt;
        }
        if (candidate.transport != remote::DiscoveryTransport::remote_pairing || candidate.pairing != remote::DiscoveryPairing::ready
            || candidate.address.empty() || !candidate.port) {
            continue;
        }
        std::string candidate_error;
        auto device = connect(candidate.address, record, candidate.port, candidate_error);
        if (startup_cancelled(should_cancel, error)) {
            return std::nullopt;
        }
        if (device) {
            return device;
        }
        failures += "\n  " + candidate.address + ":" + std::to_string(candidate.port) + ": " + candidate_error;
    }
    error = "Cannot connect to any discovered wireless address";
    error += failures;
    return std::nullopt;
}

std::optional<remote::Device> detail::open_default_device(
    const std::string& serial,
    std::string& error,
    const std::function<bool()>& should_cancel,
    const DefaultDeviceOperations& operations,
    bool physical_usb_only)
{
    error.clear();
    if (startup_cancelled(should_cancel, error)) {
        return std::nullopt;
    }
    std::vector<transport::DeviceRecord> devices;
    std::string usb_error;
    const auto status = operations.enumerate(devices, std::chrono::milliseconds(1000), should_cancel, usb_error);
    if (status == transport::UsbmuxDiscoveryStatus::cancelled || startup_cancelled(should_cancel, error)) {
        error = "Device startup cancelled";
        return std::nullopt;
    }
    if (physical_usb_only) {
        std::erase_if(devices, [](const auto& item) { return !item.is_usb(); });
    }
    if (status == transport::UsbmuxDiscoveryStatus::complete && has_usbmux_match(devices, serial)) {
        // 完整快照先应用真实 UDID/传输歧义政策；不可用快照不能冒充空列表。
        if (!remote::detail::select_usbmux_device(devices, serial, error)) {
            return std::nullopt;
        }
        if (startup_cancelled(should_cancel, error)) {
            return std::nullopt;
        }
        // 保留原 USB 连接路径及二次枚举；协议失败不能据此改连无线会话。
        auto device = operations.connect_usbmux(serial, error);
        if (startup_cancelled(should_cancel, error)) {
            return std::nullopt;
        }
        return device;
    }
    std::string warning;
    if (status != transport::UsbmuxDiscoveryStatus::complete) {
        warning = "USB device enumeration is unavailable";
        if (!usb_error.empty()) {
            warning += ": " + usb_error;
        }
        if (operations.report_warning) {
            operations.report_warning(warning);
        }
    }
    if (startup_cancelled(should_cancel, error)) {
        return std::nullopt;
    }
    auto device = operations.connect_wireless(serial, error);
    if (startup_cancelled(should_cancel, error)) {
        return std::nullopt;
    }
    if (!device && !warning.empty()) {
        error += "\n" + warning;
    }
    return device;
}

std::optional<remote::Device> open_device(
    const std::string& serial,
    const std::string& wifi,
    std::string& error,
    uint16_t wifi_port,
    const std::function<bool()>& should_cancel,
    const std::vector<wifi::PairRecord>& records,
    bool physical_usb_only)
{
    transport::IoBudget budget({ std::chrono::milliseconds(30000), should_cancel });
    const std::function<bool()> cancelled = [&budget] {
        return budget.stopped();
    };
    auto run = [&]() -> std::optional<remote::Device> {
        error.clear();
        if (startup_cancelled(cancelled, error)) {
            return std::nullopt;
        }
        if (!physical_usb_only && wifi == "auto") {
            return open_discovered_device(serial, error, cancelled, budget, records);
        }
        if (!physical_usb_only && !wifi.empty()) {
            if (!wifi_port) {
                error = "Wi-Fi port must be between 1 and 65535";
                return std::nullopt;
            }
            const auto record = load_selected_record(serial, error, cancelled, records);
            if (!record) {
                return std::nullopt;
            }
            if (startup_cancelled(cancelled, error)) {
                return std::nullopt;
            }
            auto device = remote::Device::establish_wifi_bounded(wifi, *record, budget, error, false, wifi_port);
            if (startup_cancelled(cancelled, error)) {
                return std::nullopt;
            }
            return device;
        }
        return detail::open_default_device(
            serial,
            error,
            cancelled,
            {
                transport::Usbmux::list_devices_for_discovery,
                [&budget, physical_usb_only](const std::string& selected, std::string& err) {
                    return remote::Device::establish_bounded(selected, budget, err, false, physical_usb_only);
                },
                [&](const std::string& selected, std::string& err) -> std::optional<remote::Device> {
                    if (wifi.empty() || wifi == "auto") {
                        return open_discovered_device(selected, err, cancelled, budget, records);
                    }
                    const auto record = load_selected_record(selected, err, cancelled, records);
                    if (!record) {
                        return std::nullopt;
                    }
                    return remote::Device::establish_wifi_bounded(wifi, *record, budget, err, false, wifi_port);
                },
                [](const std::string& warning) { LogWarn << std::format("Discovery warning: {}", warning); },
            },
            physical_usb_only);
    };
    auto device = run();
    if (!budget.check(error)) {
        return std::nullopt;
    }
    return device;
}
} // namespace maa::ios::remote
