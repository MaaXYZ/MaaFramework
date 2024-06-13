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
    virtual ~DeviceMgrLinux() override = default;

private:
    DeviceMgrLinux() = default;
};

MAA_TOOLKIT_NS_END

#endif
