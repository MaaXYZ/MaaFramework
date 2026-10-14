#include "Cli.h"

#include <charconv>
#include <ostream>
#include <set>

namespace maa::ios::tool
{
namespace
{
bool number(std::string_view value, int minimum, int maximum, int& result)
{
    const auto parsed = std::from_chars(value.data(), value.data() + value.size(), result);
    return parsed.ec == std::errc { } && parsed.ptr == value.data() + value.size() && result >= minimum && result <= maximum;
}

bool selected(Command command, Command left, Command right)
{
    return command == left || command == right;
}
}

bool parse(const std::vector<std::string_view>& arguments, Options& options, std::string& error)
{
    options = { };
    error.clear();
    if (arguments.empty() || (arguments.size() == 1 && (arguments.front() == "--help" || arguments.front() == "-h"))) {
        return true;
    }
    const auto command = arguments.front();
    if (command == "devices") {
        options.command = Command::Devices;
    }
    else if (command == "pair") {
        options.command = Command::Pair;
    }
    else if (command == "connect") {
        options.command = Command::Connect;
    }
    else if (command == "prepare") {
        options.command = Command::Prepare;
    }
    else {
        error = "Unknown command: " + terminal_text(command);
        return false;
    }
    if (arguments.size() == 2 && (arguments[1] == "--help" || arguments[1] == "-h")) {
        options.command = Command::Help;
        return true;
    }
    options.timeout_ms = options.command == Command::Pair ? 120000 : 3000;
    std::set<std::string_view> seen;
    for (std::size_t index = 1; index < arguments.size(); ++index) {
        auto option = arguments[index];
        if (option == "-s") {
            option = "--udid";
        }
        if (!seen.insert(option).second) {
            error = "Duplicate option: " + terminal_text(option);
            return false;
        }
        const bool devices = options.command == Command::Devices;
        const bool pair = options.command == Command::Pair;
        const bool session = selected(options.command, Command::Connect, Command::Prepare);
        const bool prepare = options.command == Command::Prepare;
        bool value_required = false;
        if (option == "--json" && (devices || options.command == Command::Connect)) {
            options.json = true;
        }
        else if (option == "--usb-only" && devices) {
            options.usb_only = true;
        }
        else if (option == "--wifi-only" && devices) {
            options.wifi_only = true;
        }
        else if (option == "--repair" && pair) {
            options.repair = true;
        }
        else if (option == "--replace" && prepare) {
            options.replace = true;
        }
        else if (option == "--wifi" && pair) {
            options.wifi = "auto";
        }
        else if (
            (option == "--udid" && !devices) || (option == "--wifi" && session) || (option == "--port" && session)
            || (option == "--timeout" && (devices || pair)) || (option == "--cache-dir" && prepare)
            || (option == "--download-timeout" && prepare) || (option == "--install-timeout" && prepare)) {
            value_required = true;
        }
        else {
            error = "Unsupported option for " + std::string(command) + ": " + terminal_text(option);
            return false;
        }
        if (!value_required) {
            continue;
        }
        if (++index == arguments.size() || arguments[index].empty() || arguments[index].starts_with("--")) {
            error = "Missing value for " + std::string(option);
            return false;
        }
        const auto value = arguments[index];
        int parsed = 0;
        if (option == "--udid") {
            options.udid = value;
        }
        else if (option == "--wifi") {
            options.wifi = value;
        }
        else if (option == "--cache-dir") {
            options.cache_directory = value;
        }
        else if (option == "--port" && number(value, 1, 65535, parsed)) {
            options.port = static_cast<unsigned short>(parsed);
        }
        else if (option == "--timeout" && number(value, pair ? 1 : 0, pair ? 300000 : 60000, parsed)) {
            options.timeout_ms = parsed;
        }
        else if (option == "--download-timeout" && number(value, 1, 3600, parsed)) {
            options.download_timeout_seconds = parsed;
        }
        else if (option == "--install-timeout" && number(value, 1, 3600, parsed)) {
            options.install_timeout_seconds = parsed;
        }
        else {
            error = "Invalid value for " + std::string(option);
            return false;
        }
    }
    if (options.usb_only && options.wifi_only) {
        error = "--usb-only and --wifi-only are mutually exclusive";
    }
    if (options.repair && !options.wifi.empty()) {
        error = "--repair is supported only for USB pairing";
    }
    if (seen.contains("--port") && (options.wifi.empty() || options.wifi == "auto")) {
        error = "--port requires an explicit --wifi address";
    }
    return error.empty();
}

void help(std::ostream& output)
{
    output << "MaaIOSDevice - MaaFramework iOS device management\n\n"
           << "  devices [--json] [--timeout MS] [--usb-only | --wifi-only]\n"
           << "  pair [-s UDID] [--wifi] [--repair] [--timeout MS]\n"
           << "  connect [-s UDID] [--wifi auto | ADDRESS] [--port PORT] [--json]\n"
           << "  prepare [-s UDID] [--wifi auto | ADDRESS] [--port PORT]\n"
           << "          [--cache-dir PATH] [--replace]\n"
           << "          [--download-timeout SECONDS] [--install-timeout SECONDS]\n\n"
           << "devices returns a snapshot, retaining all address and interface candidates.\n"
           << "pair uses USB by default. --wifi advertises a computer for phone-initiated\n"
           << "pairing and displays its one-use PIN. Confirm trust on the unlocked phone.\n"
           << "connect validates authentication, the tunnel and RSD, then closes the tunnel.\n"
           << "It starts no server; controllers create their own sessions. No disconnect command\n"
           << "is needed. Default connection selects USB, falling back to paired Wi-Fi when absent.\n"
           << "prepare downloads a verified DDI for the device and installs it natively.\n"
           << "--replace explicitly allows replacing an existing incompatible image.\n"
           << "Pairing records are shared with MaaFramework controllers, not scrctl.\n"
           << "Discovery defaults to 3000 ms, pairing to 120000 ms, connection preparation\n"
           << "has a 30-second budget. Ctrl+C cancels; interrupted installation may take effect.\n";
}

std::string terminal_text(std::string_view text)
{
    constexpr char hex[] = "0123456789abcdef";
    std::string result;
    for (unsigned char byte : text) {
        if (byte < 32 || byte == 127) {
            result += "\\x";
            result += hex[byte >> 4];
            result += hex[byte & 15];
        }
        else {
            result += static_cast<char>(byte);
        }
    }
    return result;
}

std::string json_string(std::string_view text)
{
    constexpr char hex[] = "0123456789abcdef";
    std::string result = "\"";
    for (unsigned char byte : text) {
        if (byte == '"' || byte == '\\') {
            result += '\\';
        }
        if (byte < 32) {
            result += "\\u00";
            result += hex[byte >> 4];
            result += hex[byte & 15];
        }
        else {
            result += static_cast<char>(byte);
        }
    }
    result += '"';
    return result;
}

int run(const Options& options, std::ostream& output, std::ostream& errors, const std::function<bool()>& cancelled, const Execute& execute)
{
    if (options.command == Command::Help) {
        help(output);
        return 0;
    }
    if (cancelled && cancelled()) {
        return 130;
    }
    const int result = execute(options, output, errors, cancelled);
    return cancelled && cancelled() ? 130 : result;
}
}
