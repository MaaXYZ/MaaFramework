#pragma once

#if defined(_WIN32)

#include "Conf/Conf.h"
#include "DeviceMgr.h"
#include "Utils/SingletonHolder.hpp"

#include <filesystem>
#include <ostream>

MAA_TOOLKIT_DEVICE_NS_BEGIN

class DeviceMgrWin32 : public SingletonHolder<DeviceMgrWin32>, public DeviceMgr
{
    friend class SingletonHolder<DeviceMgrWin32>;

public:
    struct Emulator
    {
        // constant
        const std::string name;
        const std::string process_keyword;
        const std::vector<std::filesystem::path> adb_relative_paths;
        const std::vector<std::string> adb_common_serials;

        // variable
        unsigned long pid = 0;
        std::string process_name;
    };

public:
    virtual ~DeviceMgrWin32() noexcept override = default;

public: // from MaaToolKitDeviceMgrAPI
    virtual size_t find_device(std::string_view adb_path = std::string_view()) override;

private:
    DeviceMgrWin32() = default;

    std::vector<Emulator> get_emulators();
};

std::ostream& operator<<(std::ostream& os, const DeviceMgrWin32::Emulator& emulator);

MAA_TOOLKIT_DEVICE_NS_END

#endif
