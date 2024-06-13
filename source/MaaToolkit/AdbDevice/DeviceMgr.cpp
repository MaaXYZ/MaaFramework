#include "DeviceMgr.h"

#include <future>
#include <filesystem>
#include <ranges>

#include "ControlUnit/ControlUnitAPI.h"
#include "LibraryHolder/ControlUnit.h"
#include "Utils/IOStream/BoostIO.hpp"
#include "Utils/Logger.h"

MAA_TOOLKIT_NS_BEGIN

std::ostream& operator<<(std::ostream& os, const DeviceMgr::Emulator& emulator)
{
    os << VAR_RAW(emulator.name) << VAR_RAW(emulator.process);
    return os;
}

bool DeviceMgr::post_find_device()
{
    LogFunc;

    if (find_device_future_.valid()) {
        LogError << "find_device_future_ is running";
        return false;
    }

    devices_ = std::nullopt;
    find_device_future_ =
        std::async(std::launch::async, [=, this]() { return find_device_impl(); });
    return true;
}

bool DeviceMgr::post_find_device_with_adb(std::filesystem::path adb_path)
{
    LogFunc;

    if (find_device_future_.valid()) {
        LogError << "find_device_future_ is running";
        return false;
    }

    devices_ = std::nullopt;
    find_device_future_ =
        std::async(std::launch::async, [=, this]() { return find_device_with_adb_impl(adb_path); });
    return true;
}

bool DeviceMgr::is_find_completed() const
{
    if (devices_.has_value()) {
        return true;
    }

    if (!find_device_future_.valid()) {
        return false;
    }

    return find_device_future_.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
}

const std::optional<std::vector<Device>>& DeviceMgr::get_devices()
{
    if (!is_find_completed()) {
        LogError << "find_device_future_ is running";
        devices_ = std::nullopt;
        return devices_;
    }
    if (find_device_future_.valid()) {
        devices_ = find_device_future_.get();
    }

    return devices_;
}

json::object
    DeviceMgr::get_adb_config(const Emulator& emulator, const std::string& adb_serial) const
{
    std::ignore = emulator;
    std::ignore = adb_serial;

    return {};
}

void DeviceMgr::set_emulator_const_data(std::unordered_map<std::string, EmulatorConstantData> data)
{
    const_data_ = std::move(data);
}

std::vector<Device> DeviceMgr::find_device_impl()
{
    std::vector<Device> result;

    auto all_emulators = find_emulators();
    for (const Emulator& e : all_emulators) {
        std::filesystem::path adb_path = get_adb_path(e.const_data, e.process.pid);

        std::vector<std::string> serials = e.const_data.adb_common_serials;

        auto requested = request_adb_serials(adb_path, json::object());

        serials.insert(
            serials.end(),
            std::make_move_iterator(requested.begin()),
            std::make_move_iterator(requested.end()));

        // Deduplication
        std::sort(serials.begin(), serials.end());
        serials.erase(std::unique(serials.begin(), serials.end()), serials.end());

        serials = check_available_adb_serials(adb_path, serials, json::object());

        for (const std::string& ser : serials) {
            Device device;
            device.name = e.name;
            device.adb_path = path_to_utf8_string(adb_path);
            device.adb_serial = ser;
            device.adb_config = get_adb_config(e, ser).to_string();
            device.adb_controller_type =
                check_adb_controller_type(device.adb_path, device.adb_serial, device.adb_config);
            result.emplace_back(std::move(device));
        }
    }

    auto env_adb = boost::process::search_path("adb");

    if (std::filesystem::exists(env_adb)) {
        auto env_adb_devices = find_device_with_adb_impl(env_adb);
        result.insert(
            result.end(),
            std::make_move_iterator(env_adb_devices.begin()),
            std::make_move_iterator(env_adb_devices.end()));
    }

    // Deduplication
    std::sort(result.begin(), result.end());
    result.erase(std::unique(result.begin(), result.end()), result.end());

    return result;
}

std::vector<Device> DeviceMgr::find_device_with_adb_impl(std::filesystem::path adb_path)
{
    std::vector<Device> result;

    auto serials = request_adb_serials(adb_path, json::object());

    for (const std::string& ser : serials) {
        Device device;
        device.adb_path = path_to_utf8_string(adb_path);
        device.name = device.adb_path;
        device.adb_serial = ser;
        device.adb_config = json::object().to_string();
        device.adb_controller_type =
            check_adb_controller_type(adb_path, device.adb_serial, device.adb_config);
        result.emplace_back(std::move(device));
    }
    return result;
}

std::vector<DeviceMgr::Emulator> DeviceMgr::find_emulators() const
{
    std::vector<Emulator> result;

    auto all_processes = list_processes();
    for (const auto& process : all_processes) {
        auto find_it = std::ranges::find_if(const_data_, [&process](const auto& pair) -> bool {
            return process.name.find(pair.second.keyword) != std::string::npos;
        });
        if (find_it == const_data_.cend()) {
            continue;
        }

        Emulator emulator {
            .name = find_it->first,
            .process = process,
            .const_data = find_it->second,
        };
        result.emplace_back(std::move(emulator));
    }

    LogInfo << VAR(result);

    return result;
}

std::filesystem::path
    DeviceMgr::get_adb_path(const EmulatorConstantData& emulator, os_pid pid) const
{
    auto path_opt = get_process_path(pid);
    if (!path_opt) {
        return {};
    }
    auto dir = path_opt->parent_path();

    for (const auto& adb_rel_path : emulator.adb_candidate_paths) {
        auto adb_path = dir / adb_rel_path;
        if (!std::filesystem::exists(adb_path)) {
            continue;
        }
        return adb_path;
    }
    return {};
}

std::vector<std::string> DeviceMgr::request_adb_serials(
    const std::filesystem::path& adb_path,
    const json::value& adb_config) const
{
    LogFunc << VAR(adb_path);

    std::string str_adb = path_to_utf8_string(adb_path);
    std::string str_config = adb_config.to_string();

    auto control_unit = AdbControlUnitLibraryHolder::create_control_unit(
        str_adb.c_str(),
        "",
        0,
        str_config.c_str(),
        "",
        nullptr,
        nullptr);

    if (!control_unit) {
        LogError << "Failed to create control unit";
        return {};
    }

    std::vector<std::string> devices;
    auto found = control_unit->find_device(devices);

    if (!found) {
        LogError << "Failed to find_device";
        return {};
    }

    return devices;
}

bool DeviceMgr::request_adb_connect(
    const std::filesystem::path& adb_path,
    const std::string& serial,
    const json::value& adb_config) const
{
    LogFunc << VAR(adb_path) << VAR(serial);

    std::string str_adb = path_to_utf8_string(adb_path);
    std::string str_serial = serial;
    std::string str_config = adb_config.to_string();

    auto control_unit = AdbControlUnitLibraryHolder::create_control_unit(
        str_adb.c_str(),
        str_serial.c_str(),
        0,
        str_config.c_str(),
        "",
        nullptr,
        nullptr);

    if (!control_unit) {
        LogError << "Failed to create control unit";
        return false;
    }

    auto connected = control_unit->connect();

    if (!connected) {
        return false;
    }

    return true;
}

std::vector<std::string> DeviceMgr::check_available_adb_serials(
    const std::filesystem::path& adb_path,
    const std::vector<std::string>& serials,
    const json::value& adb_config) const
{
    std::vector<std::string> available;
    for (const std::string& ser : serials) {
        if (!request_adb_connect(adb_path, ser, adb_config)) {
            continue;
        }
        available.emplace_back(ser);
    }
    return available;
}

MaaAdbControllerType DeviceMgr::check_adb_controller_type(
    const std::filesystem::path& adb_path,
    const std::string& adb_serial,
    const json::value& adb_config) const
{
    std::ignore = adb_path;
    std::ignore = adb_serial;
    std::ignore = adb_config;

    constexpr MaaAdbControllerType kInputType = MaaAdbControllerType_Input_Preset_AutoDetect;
    constexpr MaaAdbControllerType kScreencapType =
        MaaAdbControllerType_Screencap_FastestLosslessWay;

    return kInputType | kScreencapType;
}

MAA_TOOLKIT_NS_END
