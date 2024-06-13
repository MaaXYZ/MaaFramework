#pragma once

#include <filesystem>
#include <future>
#include <ostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <meojson/json.hpp>

#include "Conf/Conf.h"
#include "DeviceAPI.h"
#include "Utils/Platform.h"

MAA_TOOLKIT_NS_BEGIN

class DeviceMgr : public MaaToolkitDeviceMgrAPI
{
public:
    struct EmulatorConstantData
    {
        std::string keyword;
        std::vector<std::filesystem::path> adb_candidate_paths;
        std::vector<std::string> adb_common_serials;
    };

    struct Emulator
    {
        std::string name;
        ProcessInfo process;
        const EmulatorConstantData const_data;
    };

public:
    virtual ~DeviceMgr() override = default;

public: // from MaaToolkitDeviceMgrAPI
    virtual bool post_find_device() override final;
    virtual bool post_find_device_with_adb(std::filesystem::path adb_path) override final;
    virtual bool is_find_completed() const override final;
    virtual const std::optional<std::vector<Device>>& get_devices() override final;

protected:
    virtual json::object
        get_adb_config(const Emulator& emulator, const std::string& adb_serial) const;

protected:
    void set_emulator_const_data(std::unordered_map<std::string, EmulatorConstantData> data);
    std::vector<Device> find_device_impl();
    std::vector<Device> find_device_with_adb_impl(std::filesystem::path adb_path);

    std::vector<Emulator> find_emulators() const;
    std::filesystem::path get_adb_path(const EmulatorConstantData& emulator, os_pid pid) const;

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
    std::unordered_map<std::string, EmulatorConstantData> const_data_;
};

std::ostream& operator<<(std::ostream& os, const DeviceMgr::Emulator& emulator);

MAA_TOOLKIT_NS_END
