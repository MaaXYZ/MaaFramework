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
    virtual ~DeviceMgrWin32() override = default;

protected:
    virtual json::object get_adb_config(const Emulator& emulator, const std::string& adb_serial) const;

private:
    DeviceMgrWin32();

    static int get_mumu_index(const std::string& adb_serial);
};

MAA_TOOLKIT_NS_END

#endif
