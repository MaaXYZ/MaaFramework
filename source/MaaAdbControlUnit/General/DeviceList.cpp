#include "DeviceList.h"

#include <ranges>

#include "Utils/Logger.h"
#include "Utils/StringMisc.hpp"

MAA_CTRL_UNIT_NS_BEGIN

bool DeviceList::parse(const json::value& config)
{
    static const json::array kDefaultDevicesArgv = {
        "{ADB}",
        "devices",
    };

    return parse_command("Devices", config, kDefaultDevicesArgv, devices_argv_);
}

std::optional<std::vector<std::string>> DeviceList::request_devices()
{
    LogFunc;

    auto argv_opt = devices_argv_.gen(argv_replace_);
    if (!argv_opt) {
        return std::nullopt;
    }

    auto output_opt = startup_and_read_pipe(*argv_opt);
    if (!output_opt) {
        return std::nullopt;
    }

    // like:
    // List of devices attached
    // 127.0.0.1:16384 offline
    // 127.0.0.1:16416 device
    auto devices_str = std::move(output_opt).value();
    auto lines = string_split(devices_str, '\n');
    if (lines.empty()) {
        return {};
    }
    lines.erase(lines.begin()); // remove "List of devices attached"

    std::vector<std::string> devices;
    for (auto&& line : lines) {
        if (line.find("device") == std::string::npos) {
            continue;
        }
        string_trim_(line);
        devices.emplace_back(string_split(line, '\t')[0]);
    }
    LogInfo << VAR(devices);

    return devices;
}

MAA_CTRL_UNIT_NS_END
