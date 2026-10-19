#include "Operations.h"
#include "MaaFramework/MaaDef.h"
#include "decode/Decoder.h"

#include "remote/DeviceConnection.h"
#include "remote/Discovery.h"
#include "remote/Pairing.h"
#include "remote/WifiPairing.h"

namespace maa::ios::management
{
namespace
{
const char* pairing_name(remote::DiscoveryPairing pairing)
{
    switch (pairing) {
    case remote::DiscoveryPairing::ready:
        return "paired";
    case remote::DiscoveryPairing::needs_pairing:
        return "needs_pairing";
    case remote::DiscoveryPairing::ambiguous:
        return "ambiguous";
    case remote::DiscoveryPairing::invalid_advertisement:
        return "invalid_advertisement";
    case remote::DiscoveryPairing::crypto_error:
        return "crypto_error";
    default:
        return "unmatched";
    }
}
}

bool resolve_connection(Options& options, const Callbacks& callbacks, std::string& error)
{
    if (options.connection_method == MaaIOSConnectionMethod_Auto) {
        std::vector<transport::DeviceRecord> records;
        const auto status =
            transport::Usbmux::list_devices_for_discovery(records, std::chrono::milliseconds(1000), callbacks.cancelled, error);
        if (callbacks.stopped()) {
            error = "iOS management cancelled";
            return false;
        }
        std::erase_if(records, [&](const auto& record) {
            return record.connection_type != "USB" || (!options.udid.empty() && options.udid != record.udid);
        });
        if (records.size() > 1) {
            error = "Multiple USB devices connected; specify a UDID";
            return false;
        }
        options.connection_method = status == transport::UsbmuxDiscoveryStatus::complete && !records.empty() ? MaaIOSConnectionMethod_USB
                                                                                                             : MaaIOSConnectionMethod_WiFi;
        error.clear();
    }
    if (options.connection_method == MaaIOSConnectionMethod_USB) {
        options.wifi.clear();
    }
    return true;
}

Json discover(const Options& options, const Callbacks& callbacks)
{
    remote::DiscoveryOptions request;
    request.timeout = std::chrono::milliseconds(options.timeout_ms);
    request.include_usb = options.connection_method != MaaIOSConnectionMethod_WiFi;
    request.include_wifi = options.connection_method != MaaIOSConnectionMethod_USB;
    request.should_cancel = callbacks.cancelled;
    request.pairing_records = options.pairing_records;
    const auto found = remote::discover_devices(request);
    auto devices = ::json::array { };
    for (const auto& device : found.devices) {
        auto candidates = ::json::array { };
        for (const auto& c : device.candidates) {
            if (c.transport == remote::DiscoveryTransport::usbmux && c.connection_type != "USB") {
                continue;
            }
            candidates.emplace_back(
                ::json::object { { "transport", c.transport == remote::DiscoveryTransport::usbmux ? "usbmux" : "remote_pairing" },
                                 { "connection_type", c.connection_type },
                                 { "device_id", c.device_id },
                                 { "address", c.address },
                                 { "port", c.port },
                                 { "interface_index", c.interface_index },
                                 { "interface_name", c.interface_name },
                                 { "instance", c.instance },
                                 { "identifier", c.identifier },
                                 { "pairing", pairing_name(c.pairing) },
                                 { "pairing_state", static_cast<int>(c.pairing) } });
        }
        if (candidates.empty()) {
            continue;
        }
        const auto methods = MaaIOScreencapMethod_ScreenshotService | (kHaveDecoder ? MaaIOScreencapMethod_Stream : 0ULL);
        devices.emplace_back(
            ::json::object { { "udid", device.udid },
                             { "name", device.name },
                             { "candidates", std::move(candidates) },
                             { "screencap_methods", methods } });
    }
    const bool ok = !found.cancelled && (found.usb_available || found.wifi_available);
    return { { "success", ok },
             { "error", ok ? "" : (found.cancelled ? "iOS discovery cancelled" : "iOS discovery sources unavailable") },
             { "cancelled", found.cancelled },
             { "usb_available", found.usb_available },
             { "wifi_available", found.wifi_available },
             { "devices", std::move(devices) },
             { "warnings", found.warnings } };
}

Json pair(const Options& options, const Callbacks& callbacks, std::optional<wifi::PairRecord>& credential)
{
    const auto progress = [&callbacks](std::string_view message) {
        callbacks.notify({ { "type", "progress" }, { "message", message } });
    };
    remote::PairingResult result;
    if (!options.wifi.empty()) {
        remote::WifiPairingOptions request;
        request.udid = options.udid;
        request.host_name = options.host_name;
        request.timeout_ms = options.timeout_ms;
        request.should_cancel = callbacks.cancelled;
        request.progress = progress;
        request.display_pin = [&callbacks](std::string_view pin, std::string& error) {
            if (!callbacks.event) {
                error = "wireless pairing requires a PIN event callback";
                return false;
            }
            if (!callbacks.notify({ { "type", "pin" }, { "pin", pin } })) {
                error = "wireless pairing PIN was rejected by the caller";
                return false;
            }
            return true;
        };
        result = remote::pair_wifi_remote(request);
    }
    else {
        remote::UsbPairingOptions request;
        request.udid = options.udid;
        request.timeout_ms = options.timeout_ms;
        request.should_cancel = callbacks.cancelled;
        request.progress = progress;
        result = remote::pair_usb_remote(request);
    }
    if (result.ok && result.record) {
        credential = std::move(result.record);
    }
    return { { "success", result.ok }, { "error", result.error }, { "cancelled", callbacks.stopped() }, { "udid", result.udid } };
}

Json check_connection(const Options& options, const Callbacks& callbacks)
{
    std::string error;
    std::optional<remote::Device> device;
    if (options.connection_method == MaaIOSConnectionMethod_USB) {
        transport::IoBudget budget({ std::chrono::seconds(30), callbacks.cancelled });
        device = remote::Device::establish_bounded(options.udid, budget, error, false, true);
    }
    else {
        device = remote::open_device(options.udid, options.wifi, error, options.port, callbacks.cancelled, options.pairing_records);
    }
    if (!device) {
        return { { "success", false },
                 { "error", error },
                 { "cancelled", callbacks.stopped() },
                 { "verified", false },
                 { "session_open", false } };
    }
    Json result { { "success", true },
                  { "error", "" },
                  { "cancelled", false },
                  { "verified", true },
                  { "session_open", false },
                  { "udid", device->udid() },
                  { "transport", device->connection_type() },
                  { "os_version", device->property("OSVersion") },
                  { "product_type", device->property("ProductType") } };
    device.reset();
    return result;
}
} // namespace maa::ios::management
