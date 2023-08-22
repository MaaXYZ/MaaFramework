#pragma once

#if defined(_WIN32)

#include "Conf/Conf.h"
#include "DeviceMgr.h"
#include "Utils/SingletonHolder.hpp"

#include <filesystem>
#include <ostream>

#include "Utils/Platform.h"

MAA_TOOLKIT_DEVICE_NS_BEGIN

class DeviceMgrWin32 : public SingletonHolder<DeviceMgrWin32>, public DeviceMgr
{
    friend class SingletonHolder<DeviceMgrWin32>;

public:
    struct Emulator
    {
        std::string name;
        ProcessInfo process;
    };

public:
    virtual ~DeviceMgrWin32() noexcept override = default;

public: // from DeviceMgr
    virtual std::vector<Device> find_device_impl(std::string_view specified_adb) override;

private:
    DeviceMgrWin32() = default;

    std::vector<Emulator> find_emulators() const;
};

std::ostream& operator<<(std::ostream& os, const DeviceMgrWin32::Emulator& emulator);

MAA_TOOLKIT_DEVICE_NS_END

#endif
