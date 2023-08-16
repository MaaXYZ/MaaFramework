#pragma once

#if defined(__linux__)

#include "Conf/Conf.h"
#include "DeviceMgr.h"
#include "Utils/SingletonHolder.hpp"

MAA_TOOLKIT_DEVICE_NS_BEGIN

class DeviceMgrLinux : public SingletonHolder<DeviceMgrLinux>, public DeviceMgr
{
    friend class SingletonHolder<DeviceMgrLinux>;

public:
    virtual ~DeviceMgrLinux() noexcept override = default;

public: // from MaaToolKitDeviceMgrAPI
    virtual size_t find_device(std::string_view adb_path = std::string_view()) override;

private:
    DeviceMgrLinux() = default;
};

MAA_TOOLKIT_DEVICE_NS_END

#endif
