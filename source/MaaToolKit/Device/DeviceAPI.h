#pragma once

#include <string_view>

#include "Conf/Conf.h"
#include "MaaToolKit/MaaToolKitDef.h"

struct MaaToolKitDeviceMgrAPI
{
    virtual ~MaaToolKitDeviceMgrAPI() = default;

    virtual size_t find_device(std::string_view adb_path = std::string_view()) = 0;

    virtual std::string_view device_name(size_t index) const = 0;
    virtual std::string_view device_adb_path(size_t index) const = 0;
    virtual std::string_view device_adb_serial(size_t index) const = 0;
    virtual MaaAdbControllerType device_adb_controller_type(size_t index) const = 0;
    virtual std::string_view device_adb_config(size_t index) const = 0;
};
