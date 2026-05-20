#pragma once

#if defined(_WIN32)

#include <filesystem>
#include <ostream>

#include "AdbDeviceFinder.h"
#include "Common/Conf.h"
#include "MaaUtils/SingletonHolder.hpp"

MAA_TOOLKIT_NS_BEGIN

class AdbDeviceWin32Finder
    : public SingletonHolder<AdbDeviceWin32Finder>
    , public AdbDeviceFinder
{
    friend class SingletonHolder<AdbDeviceWin32Finder>;

public:
    virtual ~AdbDeviceWin32Finder() override = default;

protected:
    virtual const EmulatorConstDataMap& get_emulator_const_data() const override;
    virtual std::vector<AdbDevice> find_by_emulator_tool(const Emulator& emulator) const override;
    virtual std::vector<Emulator> find_extra_emulators() const override;

private:
    AdbDeviceWin32Finder() = default;

    std::vector<AdbDevice> find_mumu_devices(const Emulator& emulator) const;
    std::vector<AdbDevice> find_ld_devices(const Emulator& emulator) const;

    // Androws (Tencent Androws) runs as an elevated process, so its adb.exe path cannot be
    // obtained via process enumeration + OpenProcess. Fall back to Windows Registry:
    //   HKLM\SOFTWARE\Tencent\Androws                   -> InstallPath
    //   HKLM\SOFTWARE\Tencent\Androws\Androws          -> Version
    // Resolved path: {InstallPath}\Application\{Version}\adb.exe
    static std::optional<std::filesystem::path> find_androws_adb_path_from_registry();
};

MAA_TOOLKIT_NS_END

#endif
