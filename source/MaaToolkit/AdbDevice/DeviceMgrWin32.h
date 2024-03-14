#pragma once

#if defined(_WIN32)

#include <filesystem>
#include <ostream>

#include "Conf/Conf.h"
#include "DeviceMgr.h"
#include "Utils/SingletonHolder.hpp"

MAA_TOOLKIT_NS_BEGIN

class DeviceMgrWin32
    : public SingletonHolder<DeviceMgrWin32>
    , public DeviceMgr
{
    friend class SingletonHolder<DeviceMgrWin32>;

public:
    virtual ~DeviceMgrWin32() noexcept override = default;

public: // from DeviceMgr
    virtual std::vector<Device> find_device_impl() override;
    virtual std::vector<Device> find_device_with_adb_impl(std::filesystem::path adb_path) override;

private:
    DeviceMgrWin32() = default;

    std::vector<Emulator> find_emulators() const;
};

MAA_TOOLKIT_NS_END

#endif
