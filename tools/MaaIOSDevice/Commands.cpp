#include "Commands.h"

#include "remote/DeviceConnection.h"
#include "remote/Discovery.h"
#include "remote/Pairing.h"
#include "remote/WifiPairing.h"
#include "wifi/PairRecord.h"

#include <ostream>

namespace maa::ios::tool
{
namespace
{
std::string_view pairing_status(remote::DiscoveryPairing pairing)
{
    switch (pairing) {
    case remote::DiscoveryPairing::ready:
        return "paired";
    case remote::DiscoveryPairing::needs_pairing:
        return "needs_pairing";
    case remote::DiscoveryPairing::identifier_hint:
        return "identifier_hint";
    case remote::DiscoveryPairing::ambiguous:
        return "ambiguous";
    case remote::DiscoveryPairing::invalid_advertisement:
        return "invalid_advertisement";
    case remote::DiscoveryPairing::crypto_error:
        return "crypto_error";
    case remote::DiscoveryPairing::unmatched:
        return "unmatched";
    }
    return "unmatched";
}

void devices_json(const remote::DiscoveryResult& result, std::ostream& output)
{
    output << "{\"usb_available\":" << (result.usb_available ? "true" : "false")
           << ",\"wifi_available\":" << (result.wifi_available ? "true" : "false")
           << ",\"cancelled\":" << (result.cancelled ? "true" : "false") << ",\"devices\":[";
    bool first_device = true;
    for (const auto& device : result.devices) {
        if (!first_device) {
            output << ',';
        }
        first_device = false;
        output << "{\"udid\":" << json_string(device.udid) << ",\"name\":" << json_string(device.name) << ",\"candidates\":[";
        bool first_candidate = true;
        for (const auto& candidate : device.candidates) {
            if (!first_candidate) {
                output << ',';
            }
            first_candidate = false;
            output << "{\"transport\":"
                   << json_string(candidate.transport == remote::DiscoveryTransport::usbmux ? "usbmux" : "remote_pairing")
                   << ",\"connection_type\":" << json_string(candidate.connection_type) << ",\"device_id\":" << candidate.device_id
                   << ",\"address\":" << json_string(candidate.address) << ",\"port\":" << candidate.port
                   << ",\"interface_index\":" << candidate.interface_index
                   << ",\"interface_name\":" << json_string(candidate.interface_name) << ",\"instance\":" << json_string(candidate.instance)
                   << ",\"identifier\":" << json_string(candidate.identifier)
                   << ",\"pairing\":" << json_string(pairing_status(candidate.pairing)) << '}';
        }
        output << "]}";
    }
    output << "],\"warnings\":[";
    bool first_warning = true;
    for (const auto& warning : result.warnings) {
        if (!first_warning) {
            output << ',';
        }
        first_warning = false;
        output << json_string(warning);
    }
    output << "]}\n";
}

int devices(const Options& options, std::ostream& output, std::ostream& errors, const std::function<bool()>& cancelled)
{
    remote::DiscoveryOptions discovery;
    discovery.timeout = std::chrono::milliseconds(options.timeout_ms);
    discovery.should_cancel = cancelled;
    discovery.include_usb = !options.wifi_only;
    discovery.include_wifi = !options.usb_only;
    const auto result = remote::discover_devices(discovery);
    if (options.json) {
        devices_json(result, output);
    }
    else {
        output << "List of iOS devices\n";
        for (const auto& device : result.devices) {
            output << terminal_text(device.udid.empty() ? "(UDID unknown)" : device.udid);
            if (!device.name.empty()) {
                output << "  " << terminal_text(device.name);
            }
            output << '\n';
            for (const auto& candidate : device.candidates) {
                output << "  " << terminal_text(candidate.connection_type);
                if (candidate.transport == remote::DiscoveryTransport::remote_pairing) {
                    const auto address =
                        candidate.address.find(':') == std::string::npos ? candidate.address : "[" + candidate.address + "]";
                    output << "  " << terminal_text(address) << ':' << candidate.port << "  " << terminal_text(candidate.interface_name)
                           << "  " << pairing_status(candidate.pairing);
                }
                output << '\n';
            }
        }
        if (result.devices.empty()) {
            output << "No devices found\n";
        }
        for (const auto& warning : result.warnings) {
            errors << "Discovery warning: " << terminal_text(warning) << '\n';
        }
    }
    if (result.cancelled) {
        return 130;
    }
    return result.usb_available || result.wifi_available ? 0 : 1;
}

int pair(const Options& options, std::ostream& output, std::ostream& errors, const std::function<bool()>& cancelled)
{
    const auto progress = [&errors](std::string_view message) {
        errors << terminal_text(message) << '\n';
    };
    remote::PairingResult result;
    if (!options.wifi.empty()) {
        remote::WifiPairingOptions pairing;
        pairing.udid = options.udid;
        pairing.timeout_ms = options.timeout_ms;
        pairing.should_cancel = cancelled;
        pairing.progress = progress;
        pairing.display_pin = [&errors](std::string_view pin, std::string&) {
            errors << "Enter this one-use pairing code on the device: " << pin << '\n' << std::flush;
            return true;
        };
        result = remote::pair_wifi_remote(pairing);
    }
    else {
        remote::UsbPairingOptions pairing;
        pairing.udid = options.udid;
        pairing.timeout_ms = options.timeout_ms;
        pairing.allow_repair = options.repair;
        pairing.should_cancel = cancelled;
        pairing.progress = progress;
        result = remote::pair_usb_remote(pairing);
    }
    if (!result.ok) {
        errors << "Pairing failed: " << terminal_text(result.error) << '\n';
        return 1;
    }
    output << (result.reused ? "Pairing verified: " : "Pairing saved: ") << terminal_text(result.udid) << '\n';
    return 0;
}

int connect(const Options& options, std::ostream& output, std::ostream& errors, const std::function<bool()>& cancelled)
{
    std::string error;
    auto device = remote::open_device(options.udid, options.wifi, error, options.port, cancelled);
    if (!device) {
        if (options.json) {
            output << "{\"verified\":false,\"session_open\":false,\"error\":" << json_string(error) << "}\n";
        }
        else {
            errors << "Connection failed: " << terminal_text(error) << '\n';
        }
        return 1;
    }
    const auto udid = device->udid();
    const auto transport = device->connection_type();
    const auto version = device->property("OSVersion");
    const auto model = device->property("ProductType");
    device.reset();
    if (options.json) {
        output << "{\"verified\":true,\"session_open\":false,\"udid\":" << json_string(udid) << ",\"transport\":" << json_string(transport)
               << ",\"os_version\":" << json_string(version) << ",\"product_type\":" << json_string(model) << "}\n";
    }
    else {
        output << "Connection verified: " << terminal_text(udid) << "  " << terminal_text(transport) << "  " << terminal_text(model) << "  "
               << terminal_text(version) << "\nTunnel closed. Controllers establish their own sessions.\n";
    }
    return 0;
}
}

int execute(const Options& options, std::ostream& output, std::ostream& errors, const std::function<bool()>& cancelled)
{
    switch (options.command) {
    case Command::Devices:
        return devices(options, output, errors, cancelled);
    case Command::Pair:
        return pair(options, output, errors, cancelled);
    case Command::Connect:
        return connect(options, output, errors, cancelled);
    case Command::Prepare:
        return prepare(options, output, errors, cancelled);
    case Command::Help:
        help(output);
        return 0;
    }
    return 2;
}
}
