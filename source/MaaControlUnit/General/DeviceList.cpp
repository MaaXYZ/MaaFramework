#include "DeviceList.h"

#include "Utils/Logger.h"

MAA_CTRL_UNIT_NS_BEGIN

bool DeviceList::parse(const json::value& config)
{
    return parse_argv("Devices", config, devices_argv_);
}

std::optional<DeviceList::Devices> DeviceList::request_devices()
{
    LogFunc;

    auto cmd_ret = command(devices_argv_.gen(argv_replace_));

    if (!cmd_ret) {
        return std::nullopt;
    }

    // TODO: parse devices
    // auto& devices_str = cmd_ret.value();

    return devices_;
}

DeviceList::Devices DeviceList::get_devices() const
{
    return devices_;
}

MAA_CTRL_UNIT_NS_END
