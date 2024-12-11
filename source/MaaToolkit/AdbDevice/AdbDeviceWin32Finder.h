#pragma once

#if defined(_WIN32)

#include <filesystem>
#include <ostream>

#include "AdbDeviceFinder.h"
#include "Conf/Conf.h"
#include "Utils/SingletonHolder.hpp"

MAA_TOOLKIT_NS_BEGIN

class AdbDeviceWin32Finder
    : public SingletonHolder<AdbDeviceWin32Finder>
    , public AdbDeviceFinder
{
    friend class SingletonHolder<AdbDeviceWin32Finder>;

public:
    virtual ~AdbDeviceWin32Finder() override = default;

protected:
    virtual std::vector<std::string> find_adb_serials(const std::filesystem::path& adb_path, const Emulator& emulator) const override;
    virtual json::object get_adb_config(const Emulator& emulator, const std::string& adb_serial) const override;

private:
    AdbDeviceWin32Finder();

    std::vector<std::string> find_mumu_serials(const std::filesystem::path& adb_path, const Emulator& emulator) const;
    static int get_mumu_index(const std::string& adb_serial);
    static int get_ld_index(const std::string& adb_serial);
};

MAA_TOOLKIT_NS_END

#endif
