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
    struct EmulatorConstantData
    {
        std::string keyword;
        std::vector<std::filesystem::path> adb_candidate_paths;
        std::vector<std::string> adb_common_serials;
    };

    DeviceMgrWin32() = default;

    static std::filesystem::path get_adb_path(const EmulatorConstantData& emulator, os_pid pid);
    static json::object get_adb_config(const Emulator& emulator, const std::string& adb_serial);
    static int get_mumu_index(const std::string& adb_serial);

    std::vector<Emulator> find_emulators() const;

    static const std::map<std::string, EmulatorConstantData> kEmulators;
};

MAA_TOOLKIT_NS_END

#endif
