#include "DeviceInfo.h"

#include "Utils/Logger.h"

MAA_CTRL_UNIT_NS_BEGIN

bool DeviceInfo::parse(const json::value& config)
{
    static const json::array kDefaultUuidArgv = {
        "{ADB}", "-s", "{ADB_SERIAL}", "shell", "settings get secure android_id",
    };
    static const json::array kDefaultResolutionArgv = {
        "{ADB}", "-s", "{ADB_SERIAL}", "shell", "dumpsys window displays | grep -o -E cur=+[^\\ ]+ | grep -o -E [0-9]+",
    };
    static const json::array kDefaultOrientationArgv = {
        "{ADB}", "-s", "{ADB_SERIAL}", "shell", "dumpsys input | grep SurfaceOrientation | grep -m 1 -o -E [0-9]",
    };

    return parse_argv("UUID", config, kDefaultUuidArgv, uuid_argv_) &&
           parse_argv("Resolution", config, kDefaultResolutionArgv, resolution_argv_) &&
           parse_argv("Orientation", config, kDefaultOrientationArgv, orientation_argv_);
}

std::optional<std::string> DeviceInfo::request_uuid()
{
    LogFunc;

    auto cmd_ret = command(uuid_argv_.gen(argv_replace_));

    if (!cmd_ret) {
        return std::nullopt;
    }

    auto& uuid_str = cmd_ret.value();
    std::erase_if(uuid_str, [](char c) { return !std::isdigit(c) && !std::isalpha(c); });

    return uuid_str;
}

std::optional<std::pair<int, int>> DeviceInfo::request_resolution()
{
    LogFunc;

    auto cmd_ret = command(resolution_argv_.gen(argv_replace_));

    if (!cmd_ret) {
        return std::nullopt;
    }

    int width = 0, height = 0;

    std::istringstream iss(cmd_ret.value());
    iss >> width >> height;
    return std::make_pair(width, height);
}

std::optional<int> DeviceInfo::request_orientation()
{
    LogFunc;

    auto cmd_ret = command(orientation_argv_.gen(argv_replace_));

    if (!cmd_ret) {
        return std::nullopt;
    }

    const auto& s = cmd_ret.value();

    if (s.empty()) {
        return std::nullopt;
    }

    int ori = s.front() - '0';

    if (!(ori >= 0 && ori <= 3)) {
        return std::nullopt;
    }

    return ori;
}

MAA_CTRL_UNIT_NS_END
