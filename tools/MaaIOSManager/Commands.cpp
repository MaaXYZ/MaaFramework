#include "Commands.h"

#include "MaaFramework/Utility/MaaBuffer.h"
#include "MaaToolkit/IOSDevice/MaaToolkitIOSDevice.h"

#include <memory>
#include <meojson/json.hpp>
#include <ostream>

namespace maa::ios::tool
{
namespace
{
struct Callbacks
{
    std::ostream& errors;
    const std::function<bool()>& cancelled;
};

MaaBool cancelled(void* userdata)
{
    const auto& callbacks = *static_cast<Callbacks*>(userdata);
    return callbacks.cancelled && callbacks.cancelled();
}

MaaBool event(const char* text, void* userdata)
{
    auto& callbacks = *static_cast<Callbacks*>(userdata);
    const auto parsed = json::parse(text);
    if (!parsed) {
        return false;
    }
    if (parsed->get("type", std::string { }) == "pin") {
        callbacks.errors << "Enter this one-use pairing code on the device: " << terminal_text(parsed->get("pin", std::string { })) << '\n'
                         << std::flush;
    }
    else {
        const auto message = parsed->get("message", std::string { });
        if (!message.empty()) {
            callbacks.errors << terminal_text(message) << '\n';
        }
    }
    return !cancelled(userdata);
}

void show_devices(const json::value& result, std::ostream& output, std::ostream& errors)
{
    output << "List of iOS devices\n";
    const auto devices = result.get("devices", json::array { });
    for (const auto& device : devices) {
        auto udid = device.get("udid", std::string { });
        output << terminal_text(udid.empty() ? "(UDID unknown)" : udid);
        const auto name = device.get("name", std::string { });
        if (!name.empty()) {
            output << "  " << terminal_text(name);
        }
        output << '\n';
        for (const auto& candidate : device.get("candidates", json::array { })) {
            output << "  " << terminal_text(candidate.get("connection_type", std::string { }));
            if (candidate.get("transport", std::string { }) == "remote_pairing") {
                auto address = candidate.get("address", std::string { });
                if (address.find(':') != std::string::npos) {
                    address = "[" + address + "]";
                }
                output << "  " << terminal_text(address) << ':' << candidate.get("port", 0) << "  "
                       << terminal_text(candidate.get("interface_name", std::string { })) << "  "
                       << terminal_text(candidate.get("pairing", std::string { }));
            }
            output << '\n';
        }
    }
    if (devices.empty()) {
        output << "No devices found\n";
    }
    for (const auto& warning : result.get("warnings", json::array { })) {
        errors << "Discovery warning: " << terminal_text(warning.as_string()) << '\n';
    }
}

void show_result(Command command, const json::value& result, std::ostream& output, std::ostream& errors)
{
    if (command == Command::Devices) {
        show_devices(result, output, errors);
        return;
    }
    if (command == Command::Pair) {
        output << (result.get("reused", false) ? "Pairing verified: " : "Pairing saved: ")
               << terminal_text(result.get("udid", std::string { })) << '\n';
        return;
    }
    if (command == Command::Connect) {
        output << "Connection verified: " << terminal_text(result.get("udid", std::string { })) << "  "
               << terminal_text(result.get("transport", std::string { })) << "  "
               << terminal_text(result.get("product_type", std::string { })) << "  "
               << terminal_text(result.get("os_version", std::string { }))
               << "\nTunnel closed. Controllers establish their own sessions.\n";
        return;
    }
    output << "DDI preparation completed\n";
}
}

int execute(const Options& options, std::ostream& output, std::ostream& errors, const std::function<bool()>& should_cancel)
{
    if (options.command == Command::Help) {
        help(output);
        return 0;
    }
    json::value request = json::object { };
    decltype(&MaaToolkitIOSDeviceDiscover) operation = nullptr;
    switch (options.command) {
    case Command::Devices:
        operation = MaaToolkitIOSDeviceDiscover;
        request["timeout_ms"] = options.timeout_ms;
        request["usb_only"] = options.usb_only;
        request["wifi_only"] = options.wifi_only;
        break;
    case Command::Pair:
        operation = MaaToolkitIOSDevicePair;
        request["timeout_ms"] = options.timeout_ms;
        request["repair"] = options.repair;
        request["udid"] = options.udid;
        request["wifi"] = options.wifi;
        break;
    case Command::Connect:
        operation = MaaToolkitIOSDeviceCheckConnection;
        request["udid"] = options.udid;
        request["wifi"] = options.wifi;
        if (!options.wifi.empty() && options.wifi != "auto") {
            request["port"] = options.port;
        }
        break;
    case Command::Prepare:
        operation = MaaToolkitIOSDevicePrepare;
        request["udid"] = options.udid;
        request["wifi"] = options.wifi;
        if (!options.wifi.empty() && options.wifi != "auto") {
            request["port"] = options.port;
        }
        request["cache_directory"] = options.cache_directory;
        request["replace"] = options.replace;
        request["download_timeout_seconds"] = options.download_timeout_seconds;
        request["install_timeout_seconds"] = options.install_timeout_seconds;
        break;
    case Command::Help:
        return 0;
    }
    std::unique_ptr<MaaStringBuffer, decltype(&MaaStringBufferDestroy)> buffer(MaaStringBufferCreate(), MaaStringBufferDestroy);
    if (!buffer) {
        errors << "Unable to allocate operation result\n";
        return 1;
    }
    Callbacks callbacks { errors, should_cancel };
    const bool success = operation(request.to_string().c_str(), event, cancelled, &callbacks, buffer.get());
    const auto result = json::parse(MaaStringBufferGet(buffer.get()));
    if (!result || !result->is_object()) {
        errors << "Invalid iOS management result\n";
        return 1;
    }
    if (options.json) {
        output << result->to_string() << '\n';
    }
    else if (success) {
        show_result(options.command, *result, output, errors);
    }
    else {
        errors << "Device operation failed: " << terminal_text(result->get("error", std::string { })) << '\n';
        if (result->get("device_may_have_changed", false)) {
            errors << "An interrupted installation may already have taken effect; check device state before replacing again.\n";
        }
    }
    return result->get("cancelled", false) || cancelled(&callbacks) ? 130 : (success ? 0 : 1);
}
}
