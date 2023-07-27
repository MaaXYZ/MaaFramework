#include "DeviceInfo.h"

#include "Utils/Logger.hpp"

MAA_CTRL_UNIT_NS_BEGIN

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

    request_orientation();

    auto cmd_ret = command(resolution_argv_.gen(argv_replace_));

    if (!cmd_ret) {
        return std::nullopt;
    }

    std::istringstream iss(cmd_ret.value());
    int v1 = 0, v2 = 0;
    iss >> v1 >> v2;

    resolution_ = adjust_resolution_by_orientation(v1, v2);
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

DeviceResolution DeviceInfo::adjust_resolution_by_orientation(int v1, int v2)
{
    int width = 0, height = 0;
    switch (orientation_) {
    case 1:
    case 3:
        LogInfo << VAR(orientation_) << "as portrait";
        std::tie(height, width) = std::minmax(v1, v2);
        break;
    case 0:
    case 2:
    default:
        LogInfo << VAR(orientation_) << "as landscape";
        std::tie(width, height) = std::minmax(v1, v2);
        break;
    }
    return DeviceResolution { .width = width, .height = height };
}

MAA_CTRL_UNIT_NS_END
