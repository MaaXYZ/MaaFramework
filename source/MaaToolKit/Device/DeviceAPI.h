#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "Conf/Conf.h"
#include "MaaToolKit/MaaToolKitDef.h"

struct Device
{
    std::string name;
    std::string adb_path;
    std::vector<std::string> adb_serials;
    MaaAdbControllerType adb_controller_type = MaaAdbControllerType_Invalid;
    std::string adb_config;
};

struct MaaToolKitDeviceMgrAPI
{
    virtual ~MaaToolKitDeviceMgrAPI() = default;

    virtual size_t find_device(std::string_view specified_adb = std::string_view()) = 0;
    virtual const std::vector<Device>& get_devices() = 0;
};
