#include "DeviceMgr.h"

#include <future>

#include "ControlUnit/ControlUnitAPI.h"
#include "LibraryHolder/ControlUnit.h"
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
    constexpr MaaAdbControllerType kScreencapType = MaaAdbControllerType_Screencap_FastestWay;

    return kInputType | kScreencapType;
}

MAA_TOOLKIT_NS_END
