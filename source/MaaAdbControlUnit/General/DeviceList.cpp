#include "DeviceList.h"

#include "Utils/Logger.h"
#include "Utils/Ranges.hpp"
#include "Utils/StringMisc.hpp"

MAA_ADB_CTRL_UNIT_NS_BEGIN

bool DeviceList::parse(const json::value& config)
{
    return parse_argv("Devices", config, devices_argv_);
}

std::optional<DeviceList::Devices> DeviceList::request_devices()
{
    LogFunc;

    devices_.clear();
    auto cmd_ret = command(devices_argv_.gen(argv_replace_));

    if (!cmd_ret) {
        return std::nullopt;
    }

    // like:
    // List of devices attached
    // 127.0.0.1:16384 offline
    // 127.0.0.1:16416 device
    auto devices_str = std::move(cmd_ret).value();
    auto lines = string_split(devices_str, '\n');
    if (lines.empty()) {
        return {};
    }
    lines.erase(lines.begin()); // remove "List of devices attached"

    for (auto&& line : lines) {
        if (line.find("device") == std::string::npos) {
            continue;
        }
        string_trim_(line);
        devices_.emplace_back(string_split(line, '\t')[0]);
    }
    LogInfo << VAR(devices_);

    return devices_;
}

DeviceList::Devices DeviceList::get_devices() const
{
    return devices_;
}

MAA_ADB_CTRL_UNIT_NS_END
