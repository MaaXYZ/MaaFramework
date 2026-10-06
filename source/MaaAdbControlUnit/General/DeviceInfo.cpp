#include "DeviceInfo.h"

#include <charconv>

#include "MaaUtils/Logger.h"
#include "MaaUtils/Uuid.h"

MAA_CTRL_UNIT_NS_BEGIN

bool DeviceInfo::parse(const json::value& config)
{
    static const json::array kDefaultUuidArgv = {
        "{ADB}", "-s", "{ADB_SERIAL}", "shell", "settings get secure android_id",
    };
    static const json::array kDefaultResolutionArgv = {
        "{ADB}", "-s", "{ADB_SERIAL}", "shell", "dumpsys window displays | grep DisplayFrames | tail -n 1 | grep -o -E [0-9]+",
    };
    // Android 12-:   "SurfaceOrientation: 1"
    // Android 13/14: "InputDeviceOrientation: 1"
    // Android 15+:   "InputDeviceOrientation: Rotation90"
    static const json::array kDefaultOrientationArgv = {
        "{ADB}",
        "-s",
        "{ADB_SERIAL}",
        "shell",
        "dumpsys input | grep -e SurfaceOrientation -e InputDeviceOrientation | tail -n 1 | grep -m 1 -o -E [0-9]+; true",
    };

    return parse_command("UUID", config, kDefaultUuidArgv, uuid_argv_)
           && parse_command("Resolution", config, kDefaultResolutionArgv, resolution_argv_)
           && parse_command("Orientation", config, kDefaultOrientationArgv, orientation_argv_);
}

std::optional<std::string> DeviceInfo::request_uuid()
{
    LogFunc;

    auto argv_opt = uuid_argv_.gen(argv_replace_);
    if (!argv_opt) {
        return std::nullopt;
    }

    auto output_opt = startup_and_read_pipe(*argv_opt);
    if (!output_opt) {
        return make_uuid();
    }

    auto& uuid_str = output_opt.value();
    std::erase_if(uuid_str, [](unsigned char c) { return !std::isdigit(c) && !std::isalpha(c); });

    return uuid_str;
}

std::optional<DeviceInfo::DisplayInfo> DeviceInfo::request_resolution()
{
    LogFunc;

    auto argv_opt = resolution_argv_.gen(argv_replace_);
    if (!argv_opt) {
        return std::nullopt;
    }

    auto output_opt = startup_and_read_pipe(*argv_opt);
    if (!output_opt) {
        return std::nullopt;
    }

    DisplayInfo info;

    std::istringstream iss(output_opt.value());
    iss >> info.w >> info.h >> info.r;

    return info;
}

std::optional<int> DeviceInfo::request_orientation()
{
    LogFunc;

    auto argv_opt = orientation_argv_.gen(argv_replace_);
    if (!argv_opt) {
        return std::nullopt;
    }

    auto output_opt = startup_and_read_pipe(*argv_opt);
    if (!output_opt) {
        return std::nullopt;
    }

    const auto& s = output_opt.value();

    auto pos = s.find_first_of("0123456789");
    if (pos == std::string::npos) {
        return std::nullopt;
    }

    int value = 0;
    auto [ptr, ec] = std::from_chars(s.data() + pos, s.data() + s.size(), value);
    if (ec != std::errc {}) {
        return std::nullopt;
    }

    // 0-3: rotation enum (Android 14-), 90/180/270: degrees (Android 15+ "Rotation90")
    switch (value) {
    case 0:
    case 1:
    case 2:
    case 3:
        return value;
    case 90:
        return 1;
    case 180:
        return 2;
    case 270:
        return 3;
    default:
        LogWarn << "unknown orientation" << VAR(s);
        return std::nullopt;
    }
}

MAA_CTRL_UNIT_NS_END
