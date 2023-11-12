#pragma once

#include "Conf/Conf.h"
#include "DeviceAPI.h"
#include "Utils/Platform.h"

#include <filesystem>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

#include <meojson/json.hpp>

MAA_TOOLKIT_DEVICE_NS_BEGIN

class DeviceMgr : public MaaToolKitDeviceMgrAPI
{
public:
    virtual ~DeviceMgr() noexcept override = default;

public: // from MaaToolKitDeviceMgrAPI
    struct Emulator
    {
        std::string name;
        ProcessInfo process;
    };

    virtual size_t find_device() override final;
    virtual size_t find_device_with_adb(std::string_view adb_path) override final;
    virtual const std::vector<Device>& get_devices() const override final { return devices_; };

protected:
    virtual std::vector<Device> find_device_impl() = 0;
    virtual std::vector<Device> find_device_with_adb_impl(std::string_view adb_path) = 0;

protected:
    std::vector<std::string> request_adb_serials(const std::filesystem::path& adb_path,
                                                 const json::value& adb_config) const;
    bool request_adb_connect(const std::filesystem::path& adb_path, const std::string& serial,
                             const json::value& adb_config) const;
    std::vector<std::string> check_available_adb_serials(const std::filesystem::path& adb_path,
                                                         const std::vector<std::string>& serials,
                                                         const json::value& adb_config) const;
    MaaAdbControllerType check_adb_controller_type(const std::filesystem::path& adb_path, const std::string& adb_serial,
                                                   const json::value& adb_config) const;

private:
    std::vector<Device> devices_;
};

std::ostream& operator<<(std::ostream& os, const DeviceMgr::Emulator& emulator);

MAA_TOOLKIT_DEVICE_NS_END
