#include "AdbDeviceFinder.h"

#include <filesystem>
#include <ranges>

#include "ControlUnit/ControlUnitAPI.h"
#include "LibraryHolder/ControlUnit.h"
#include "MaaUtils/IOStream/BoostIO.hpp"
#include "MaaUtils/Logger.h"

MAA_TOOLKIT_NS_BEGIN

std::ostream& operator<<(std::ostream& os, const AdbDeviceFinder::Emulator& emulator)
{
    os << VAR_RAW(emulator.name) << VAR_RAW(emulator.process);
    return os;
}

std::vector<AdbDevice> AdbDeviceFinder::find() const
{
    LogFunc;

    std::vector<AdbDevice> result;

    auto all_emulators = find_emulators();
    for (const Emulator& e : all_emulators) {
        std::filesystem::path adb_path = get_adb_path(e.const_data, e.process.pid);
        std::vector<std::string> serials = find_adb_serials(adb_path, e);

        for (const std::string& ser : serials) {
            AdbDevice device;
            device.name = e.name;
            device.adb_path = adb_path;
            device.serial = ser;
            device.screencap_methods = MaaAdbScreencapMethod_Default;
            device.input_methods = MaaAdbInputMethod_Default;
            device.config = {};

            request_device_config(e, device);

            result.emplace_back(std::move(device));
        }
    }

    auto env_adb = boost::process::search_path("adb");

    if (std::filesystem::exists(env_adb)) {
        auto env_adb_devices = find_specified(env_adb);
        result.insert(result.end(), std::make_move_iterator(env_adb_devices.begin()), std::make_move_iterator(env_adb_devices.end()));
    }

    LogInfo << VAR(result);
    return result;
}

std::vector<AdbDevice> AdbDeviceFinder::find_specified(const std::filesystem::path& adb_path) const
{
    LogFunc << VAR(adb_path);

    std::vector<AdbDevice> result;

    auto serials = find_serials_by_adb_command(adb_path);

    for (const std::string& ser : serials) {
        AdbDevice device;
        device.name = path_to_utf8_string(adb_path);
        device.adb_path = adb_path;
        device.serial = ser;
        device.screencap_methods = MaaAdbScreencapMethod_Default;
        device.input_methods = MaaAdbInputMethod_Default;
        device.config = {};

        request_device_config({}, device);

        result.emplace_back(std::move(device));
    }

    LogInfo << VAR(result);
    return result;
}

std::vector<std::string> AdbDeviceFinder::find_adb_serials(const std::filesystem::path& adb_path, const Emulator& emulator) const
{
    auto serials = emulator.const_data.adb_common_serials;

    auto requested = find_serials_by_adb_command(adb_path);

    serials.insert(serials.end(), std::make_move_iterator(requested.begin()), std::make_move_iterator(requested.end()));

    // Deduplication
    std::sort(serials.begin(), serials.end());
    serials.erase(std::unique(serials.begin(), serials.end()), serials.end());

    serials = check_available_adb_serials(adb_path, serials);

    return serials;
}

void AdbDeviceFinder::set_emulator_const_data(std::unordered_map<std::string, EmulatorConstantData> data)
{
    const_data_ = std::move(data);
}

std::vector<AdbDeviceFinder::Emulator> AdbDeviceFinder::find_emulators() const
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

std::filesystem::path AdbDeviceFinder::get_adb_path(const EmulatorConstantData& emulator, os_pid pid) const
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
        return std::filesystem::canonical(adb_path);
    }
    return {};
}

std::vector<std::string> AdbDeviceFinder::find_serials_by_adb_command(const std::filesystem::path& adb_path) const
{
    LogFunc << VAR(adb_path);

    std::string str_adb = path_to_utf8_string(adb_path);

    auto control_unit =
        AdbControlUnitLibraryHolder::create_control_unit(str_adb.c_str(), "", MaaAdbScreencapMethod_None, MaaAdbInputMethod_None, "{}", "");

    if (!control_unit) {
        LogError << "Failed to create control unit";
        return {};
    }

    std::vector<std::string> devices;
    auto found = control_unit->find_device(devices);

    LogDebug << VAR(devices);

    if (!found) {
        LogError << "Failed to find_device";
        return {};
    }

    return devices;
}

bool AdbDeviceFinder::request_adb_connect(const std::filesystem::path& adb_path, const std::string& serial) const
{
    LogFunc << VAR(adb_path) << VAR(serial);

    std::string str_adb = path_to_utf8_string(adb_path);
    std::string str_serial = serial;

    auto control_unit = AdbControlUnitLibraryHolder::create_control_unit(
        str_adb.c_str(),
        str_serial.c_str(),
        MaaAdbScreencapMethod_None,
        MaaAdbInputMethod_None,
        "{}",
        "");

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

std::vector<std::string>
    AdbDeviceFinder::check_available_adb_serials(const std::filesystem::path& adb_path, const std::vector<std::string>& serials) const
{
    LogFunc;

    std::vector<std::string> available;
    for (const std::string& ser : serials) {
        if (!request_adb_connect(adb_path, ser)) {
            continue;
        }
        available.emplace_back(ser);
    }
    return available;
}

bool request_waydroid_config(std::shared_ptr<MAA_CTRL_UNIT_NS::AdbControlUnitAPI> control_unit, AdbDevice& device)
{
    if (!control_unit) {
        return false;
    }

    std::string output;
    bool ret = control_unit->shell("getprop | grep ro.product.brand", output);
    if (!ret) {
        return false;
    }
    if (output.find("waydroid") == std::string::npos) {
        return false;
    }

    auto& command = device.config["command"];

    command["StartApp"] = json::array {
        "{ADB}", "-s", "{ADB_SERIAL}", "shell", "monkey -p {INTENT} --pct-syskeys 0 1",
    };
    command["StartActivity"] = json::array {
        "{ADB}", "-s", "{ADB_SERIAL}", "shell", "am start -n {INTENT} --windowingMode 4",
    };

    LogInfo << "waydroid" << VAR(device.adb_path) << VAR(device.serial) << VAR(device.config);
    return true;
}

void AdbDeviceFinder::request_device_config(const Emulator& emulator, AdbDevice& device) const
{
    std::ignore = emulator;

    LogFunc << VAR(device.adb_path) << VAR(device.serial);

    std::string str_adb = path_to_utf8_string(device.adb_path);
    std::string str_serial = device.serial;

    auto control_unit = AdbControlUnitLibraryHolder::create_control_unit(
        str_adb.c_str(),
        str_serial.c_str(),
        MaaAdbScreencapMethod_None,
        MaaAdbInputMethod_None,
        "{}",
        "");

    if (!control_unit) {
        LogError << "Failed to create control unit";
        return;
    }
    if (!control_unit->connect()) {
        LogError << "Failed to connect";
        return;
    }

    if (request_waydroid_config(control_unit, device)) {
        return;
    }
}

MAA_TOOLKIT_NS_END
