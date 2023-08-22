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

public: // from DeviceMgr
    virtual std::vector<Device> find_device_impl(std::string_view specified_adb) override;

private:
    DeviceMgrLinux() = default;
};

MAA_TOOLKIT_DEVICE_NS_END

#endif
