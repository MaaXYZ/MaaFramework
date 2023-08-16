#pragma once

#if defined(_WIN32)

#include "Conf/Conf.h"
#include "DeviceMgr.h"
#include "Utils/SingletonHolder.hpp"

MAA_TOOLKIT_DEVICE_NS_BEGIN

class DeviceMgrWin32 : public SingletonHolder<DeviceMgrWin32>, public DeviceMgr
{
    friend class SingletonHolder<DeviceMgrWin32>;

public:
    virtual ~DeviceMgrWin32() noexcept override = default;

public: // from MaaToolKitDeviceMgrAPI
    virtual size_t find_device(std::string_view adb_path = std::string_view()) override;

private:
    DeviceMgrWin32() = default;
};

MAA_TOOLKIT_DEVICE_NS_END

#endif
