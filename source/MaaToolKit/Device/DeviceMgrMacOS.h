#pragma once

#if defined(__APPLE__)

#include "Conf/Conf.h"
#include "DeviceMgr.h"
#include "Utils/SingletonHolder.hpp"

MAA_TOOLKIT_DEVICE_NS_BEGIN

class DeviceMgrMacOS : public SingletonHolder<DeviceMgrMacOS>, public DeviceMgr
{
    friend class SingletonHolder<DeviceMgrMacOS>;

public:
    virtual ~DeviceMgrMacOS() noexcept override = default;

public: // from MaaToolKitDeviceMgrAPI
    virtual size_t find_device(std::string_view adb_path = std::string_view()) override;

private:
    DeviceMgrMacOS() = default;
};

MAA_TOOLKIT_DEVICE_NS_END

#endif
