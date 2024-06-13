#pragma once

#if defined(__APPLE__)

#include "Conf/Conf.h"
#include "DeviceMgr.h"
#include "Utils/SingletonHolder.hpp"

MAA_TOOLKIT_NS_BEGIN

class DeviceMgrMacOS
    : public SingletonHolder<DeviceMgrMacOS>
    , public DeviceMgr
{
    friend class SingletonHolder<DeviceMgrMacOS>;

public:
    virtual ~DeviceMgrMacOS() override = default;

private:
    DeviceMgrMacOS();
};

MAA_TOOLKIT_NS_END

#endif
