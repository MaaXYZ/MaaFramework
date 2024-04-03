#pragma once

#include <filesystem>
#include <future>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

#include <meojson/json.hpp>

#include "Conf/Conf.h"
#include "DeviceAPI.h"
#include "Utils/Platform.h"

MAA_TOOLKIT_NS_BEGIN

class DeviceMgr : public MaaToolkitDeviceMgrAPI
{
public:
    virtual ~DeviceMgr() noexcept override = default;

public: // from MaaToolkitDeviceMgrAPI
    struct Emulator
    {
        std::string name;
        ProcessInfo process;
    };

    virtual bool post_find_device() override final;
    virtual bool post_find_device_with_adb(std::filesystem::path adb_path) override final;
    virtual bool is_find_completed() const override final;
    virtual const std::optional<std::vector<Device>>& get_devices() override final;

protected:
    virtual std::vector<Device> find_device_impl() = 0;
    virtual std::vector<Device> find_device_with_adb_impl(std::filesystem::path adb_path) = 0;

protected:
    std::vector<std::string> request_adb_serials(
        const std::filesystem::path& adb_path,
        const json::value& adb_config) const;
    bool request_adb_connect(
        const std::filesystem::path& adb_path,
        const std::string& serial,
        const json::value& adb_config) const;
    std::vector<std::string> check_available_adb_serials(
        const std::filesystem::path& adb_path,
        const std::vector<std::string>& serials,
        const json::value& adb_config) const;
    MaaAdbControllerType check_adb_controller_type(
        const std::filesystem::path& adb_path,
        const std::string& adb_serial,
        const json::value& adb_config) const;

private:
    std::optional<std::vector<Device>> devices_;
    std::future<std::vector<Device>> find_device_future_;
};

std::ostream& operator<<(std::ostream& os, const DeviceMgr::Emulator& emulator);

MAA_TOOLKIT_NS_END
