#pragma once

#include "Conf/Conf.h"
#include "DeviceAPI.h"

#include <vector>
#include <string>
#include <string_view>

MAA_TOOLKIT_DEVICE_NS_BEGIN

class DeviceMgr : public MaaToolKitDeviceMgrAPI
{

public:
    virtual ~DeviceMgr() noexcept override = default;

public: // from MaaToolKitDeviceMgrAPI
    virtual size_t find_device(std::string_view adb_path = std::string_view()) override = 0;

    virtual std::string_view device_name(size_t index) const override final;
    virtual std::string_view device_adb_path(size_t index) const override final;
    virtual std::string_view device_adb_serial(size_t index) const override final;
    virtual MaaAdbControllerType device_adb_controller_type(size_t index) const override final;
    virtual std::string_view device_adb_config(size_t index) const override final;

protected:
    struct Device
    {
        std::string name;
        std::string adb_path;
        std::string adb_serial;
        MaaAdbControllerType adb_controller_type = MaaAdbControllerType_Invalid;
        std::string adb_config;
    };

    std::vector<Device> devices_;
};

MAA_TOOLKIT_DEVICE_NS_END
