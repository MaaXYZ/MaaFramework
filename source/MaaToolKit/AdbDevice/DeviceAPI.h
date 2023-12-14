#pragma once

#include <ostream>
#include <string>
#include <string_view>
#include <vector>

#include "Conf/Conf.h"
#include "MaaToolKit/MaaToolKitDef.h"

struct Device
{
    std::string name;
    std::string adb_path;
    std::string adb_serial;
    MaaAdbControllerType adb_controller_type = MaaAdbControllerType_Invalid;
    std::string adb_config;
};

inline std::ostream& operator<<(std::ostream& os, const Device& device)
{
#define VAR_TMP(x) "[" << #x << "=" << (x) << "] "

    os << VAR_TMP(device.name) << VAR_TMP(device.adb_path) << VAR_TMP(device.adb_serial)
       << VAR_TMP(device.adb_controller_type) << VAR_TMP(device.adb_config);
    return os;

#undef VAR_TMP
}

struct MaaToolKitDeviceMgrAPI
{
    virtual ~MaaToolKitDeviceMgrAPI() = default;

    virtual size_t find_device() = 0;
    virtual size_t find_device_with_adb(std::string_view adb_path) = 0;
    virtual const std::vector<Device>& get_devices() const = 0;
};
