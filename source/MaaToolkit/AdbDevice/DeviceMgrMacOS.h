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
    virtual ~DeviceMgrMacOS() noexcept override = default;

public: // from DeviceMgr
    virtual std::vector<Device> find_device_impl() override;
    virtual std::vector<Device> find_device_with_adb_impl(std::filesystem::path adb_path) override;

private:
    DeviceMgrMacOS() = default;

    std::vector<Emulator> find_emulators() const;
};

MAA_TOOLKIT_NS_END

#endif
