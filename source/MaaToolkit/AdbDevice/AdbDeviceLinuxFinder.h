#pragma once

#if defined(__linux__)

#include "AdbDeviceFinder.h"
#include "Conf/Conf.h"
#include "Utils/SingletonHolder.hpp"

MAA_TOOLKIT_NS_BEGIN

class AdbDeviceLinuxFinder
    : public SingletonHolder<AdbDeviceLinuxFinder>
    , public AdbDeviceFinder
{
    friend class SingletonHolder<AdbDeviceLinuxFinder>;

public:
    virtual ~AdbDeviceLinuxFinder() override = default;

private:
    AdbDeviceLinuxFinder() = default;
};

MAA_TOOLKIT_NS_END

#endif
