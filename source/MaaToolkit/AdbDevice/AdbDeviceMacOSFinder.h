#pragma once

#if defined(__APPLE__)

#include "AdbDeviceFinder.h"
#include "Common/Conf.h"
#include "MaaUtils/SingletonHolder.hpp"

MAA_TOOLKIT_NS_BEGIN

class AdbDeviceMacOSFinder
    : public SingletonHolder<AdbDeviceMacOSFinder>
    , public AdbDeviceFinder
{
    friend class SingletonHolder<AdbDeviceMacOSFinder>;

public:
    virtual ~AdbDeviceMacOSFinder() override = default;

protected:
    virtual const EmulatorConstDataMap& get_emulator_const_data() const override;

private:
    AdbDeviceMacOSFinder() = default;
};

MAA_TOOLKIT_NS_END

#endif
