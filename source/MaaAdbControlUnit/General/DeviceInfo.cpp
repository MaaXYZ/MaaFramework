#include "DeviceInfo.h"

#include "Utils/Logger.h"

MAA_ADB_CTRL_UNIT_NS_BEGIN

bool DeviceInfo::parse(const json::value& config)
{
    return parse_argv("UUID", config, uuid_argv_) && parse_argv("Resolution", config, resolution_argv_) &&
           parse_argv("Orientation", config, orientation_argv_);
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

    uuid_ = std::move(uuid_str);
    return uuid_;
}

std::optional<DeviceResolution> DeviceInfo::request_resolution()
{
    LogFunc;

    auto cmd_ret = command(resolution_argv_.gen(argv_replace_));

    if (!cmd_ret) {
        return std::nullopt;
    }

    std::istringstream iss(cmd_ret.value());
    iss >> resolution_.width >> resolution_.height;
    return resolution_;
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
        orientation_ = -1;
        return std::nullopt;
    }

    orientation_ = ori;
    return orientation_;
}

MAA_ADB_CTRL_UNIT_NS_END
