#pragma once

#if defined(__APPLE__)

#include "AdbDeviceFinder.h"
#include "Conf/Conf.h"
#include "Utils/SingletonHolder.hpp"

MAA_TOOLKIT_NS_BEGIN

class AdbDeviceMacOSFinder
    : public SingletonHolder<AdbDeviceMacOSFinder>
    , public AdbDeviceFinder
{
    friend class SingletonHolder<AdbDeviceMacOSFinder>;

public:
    virtual ~AdbDeviceMacOSFinder() override = default;

private:
    AdbDeviceMacOSFinder();
};

MAA_TOOLKIT_NS_END

#endif
