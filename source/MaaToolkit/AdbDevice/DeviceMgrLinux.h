#pragma once

#if defined(__linux__)

#include "Conf/Conf.h"
#include "DeviceMgr.h"
#include "Utils/SingletonHolder.hpp"

MAA_TOOLKIT_NS_BEGIN

class DeviceMgrLinux
    : public SingletonHolder<DeviceMgrLinux>
    , public DeviceMgr
{
    friend class SingletonHolder<DeviceMgrLinux>;

public:
    virtual ~DeviceMgrLinux() noexcept override = default;

public: // from DeviceMgr
    virtual std::vector<Device> find_device_impl() override;
    virtual std::vector<Device> find_device_with_adb_impl(std::filesystem::path adb_path) override;

private:
    DeviceMgrLinux() = default;
};

MAA_TOOLKIT_NS_END

#endif
