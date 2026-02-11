#pragma once

#if defined(__linux__)

#include "AdbDeviceFinder.h"
#include "Common/Conf.h"
#include "MaaUtils/SingletonHolder.hpp"

MAA_TOOLKIT_NS_BEGIN

class AdbDeviceLinuxFinder
    : public SingletonHolder<AdbDeviceLinuxFinder>
    , public AdbDeviceFinder
{
    friend class SingletonHolder<AdbDeviceLinuxFinder>;

public:
    virtual ~AdbDeviceLinuxFinder() override = default;

protected:
    virtual const EmulatorConstDataMap& get_emulator_const_data() const override;

private:
    AdbDeviceLinuxFinder() = default;
};

MAA_TOOLKIT_NS_END

#endif
