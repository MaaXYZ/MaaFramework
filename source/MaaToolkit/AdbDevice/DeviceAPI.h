#pragma once

#include <filesystem>
#include <ostream>
#include <string>
#include <vector>

#include "Conf/Conf.h"
#include "MaaToolkit/MaaToolkitDef.h"

struct Device
{
    std::string name;
    std::string adb_path;
    std::string adb_serial;
    MaaAdbControllerType adb_controller_type = MaaAdbControllerType_Invalid;
    std::string adb_config;

    bool operator<(const Device& rhs) const
    {
        if (name != rhs.name) {
            return name < rhs.name;
        }
        else if (adb_path != rhs.adb_path) {
            return adb_path < rhs.adb_path;
        }
        else if (adb_serial != rhs.adb_serial) {
            return adb_serial < rhs.adb_serial;
        }
        else if (adb_controller_type != rhs.adb_controller_type) {
            return adb_controller_type < rhs.adb_controller_type;
        }
        else {
            return adb_config < rhs.adb_config;
        }
    }

    bool operator==(const Device& rhs) const
    {
        return name == rhs.name && adb_path == rhs.adb_path && adb_serial == rhs.adb_serial
               && adb_controller_type == rhs.adb_controller_type && adb_config == rhs.adb_config;
    }
};

inline std::ostream& operator<<(std::ostream& os, const Device& device)
{
#define VAR_TMP(x) "[" << #x << "=" << (x) << "] "

    os << VAR_TMP(device.name) << VAR_TMP(device.adb_path) << VAR_TMP(device.adb_serial)
       << VAR_TMP(device.adb_controller_type) << VAR_TMP(device.adb_config);
    return os;

#undef VAR_TMP
}

struct MaaToolkitDeviceMgrAPI
{
    virtual ~MaaToolkitDeviceMgrAPI() = default;

    virtual bool post_find_device() = 0;
    virtual bool post_find_device_with_adb(std::filesystem::path adb_path) = 0;
    virtual bool is_find_completed() const = 0;
    virtual const std::optional<std::vector<Device>>& get_devices() = 0;
};
