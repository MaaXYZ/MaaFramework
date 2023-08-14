#pragma once

#include "Conf/Conf.h"
#include "DeviceAPI.h"
#include "Utils/SingletonHolder.hpp"

MAA_TOOLKIT_DEVICE_NS_BEGIN

class DeviceMgrMacOS : public SingletonHolder<DeviceMgrMacOS>, public MaaToolKitDeviceMgrAPI
{
    friend class SingletonHolder<DeviceMgrMacOS>;

public:
    virtual ~DeviceMgrMacOS() noexcept override = default;

public: // from MaaToolKitDeviceMgrAPI
    virtual size_t find_device() override;

    virtual std::string_view device_name(size_t index) const override;
    virtual std::string_view device_adb_path(size_t index) const override;
    virtual std::string_view device_adb_serial(size_t index) const override;
    virtual MaaAdbControllerType device_adb_controller_type(size_t index) const override;
    virtual std::string_view device_adb_config(size_t index) const override;

private:
    DeviceMgrMacOS() = default;
};

MAA_TOOLKIT_DEVICE_NS_END
