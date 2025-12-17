#include "AdbDeviceFinder.h"

#include <filesystem>
#include <ranges>
#include <unordered_set>

#include "ControlUnit/ControlUnitAPI.h"
#include "LibraryHolder/ControlUnit.h"
#include "MaaUtils/IOStream/BoostIO.hpp"
#include "MaaUtils/Logger.h"

MAA_TOOLKIT_NS_BEGIN

std::vector<AdbDevice> AdbDeviceFinder::find() const
{
    LogFunc;

    std::vector<AdbDevice> result;
    std::unordered_set<std::string> accurate_serials;

    auto all_emulators = find_emulators();
    for (const Emulator& e : all_emulators) {
        auto res = find_by_emulator_tool(e);
        bool found = !res.empty();
        for (auto& dev : res) {
            accurate_serials.emplace(dev.serial);
            result.emplace_back(std::move(dev));
        }
        if (found) {
            continue;
        }

        if (e.adb_path.empty() || !std::filesystem::exists(e.adb_path)) {
            LogWarn << "adb_path is empty or does not exist" << VAR(e.adb_path);
            continue;
        }

        res = find_specified(e.adb_path, accurate_serials, e);
        for (auto& dev : res) {
            if (accurate_serials.count(dev.serial)) {
                continue;
            }
            result.emplace_back(std::move(dev));
        }
    }

    if (auto env_adb = boost::process::search_path("adb"); std::filesystem::exists(env_adb)) {
        auto res = find_specified(env_adb, accurate_serials);
        for (auto& dev : res) {
            if (accurate_serials.count(dev.serial)) {
                continue;
            }
            result.emplace_back(std::move(dev));
        }
    }

    LogInfo << VAR(result);
    return result;
}

std::vector<AdbDevice> AdbDeviceFinder::find_specified(
    const std::filesystem::path& adb_path,
    const std::unordered_set<std::string>& exclude_serials,
    const Emulator& emulator) const
{
    LogFunc << VAR(adb_path);

    std::vector<AdbDevice> result;

    auto serials = find_serials_by_adb_command(adb_path);

    for (const std::string& ser : serials) {
        if (exclude_serials.count(ser)) {
            LogInfo << "skip excluded serial" << VAR(ser);
            continue;
        }
        auto res_opt = try_device(adb_path, ser, emulator);
        if (!res_opt) {
            continue;
        }
        result.emplace_back(std::move(*res_opt));
    }

    LogInfo << VAR(result);
    return result;
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

    LogInfo << "waydroid" << VAR(device);
    return true;
}

std::optional<AdbDevice>
    AdbDeviceFinder::try_device(const std::filesystem::path& adb_path, const std::string& serial, const Emulator& emulator) const
{
    LogFunc << VAR(adb_path) << VAR(serial);

    std::string str_adb = path_to_utf8_string(adb_path);

    auto control_unit = AdbControlUnitLibraryHolder::create_control_unit(
        str_adb.c_str(),
        serial.c_str(),
        MaaAdbScreencapMethod_None,
        MaaAdbInputMethod_None,
        "{}",
        "");

    if (!control_unit) {
        LogError << "Failed to create control unit";
        return std::nullopt;
    }
    if (!control_unit->connect()) {
        LogError << "Failed to connect" << VAR(serial);
        return std::nullopt;
    }

    AdbDevice device;
    device.name = std::format("{}-{}", serial, emulator.name.empty() ? path_to_utf8_string(adb_path) : emulator.name);
    device.adb_path = adb_path;
    device.serial = serial;
    device.screencap_methods = MaaAdbScreencapMethod_Default;
    device.input_methods = MaaAdbInputMethod_Default;
    device.config = {};

    if (request_waydroid_config(control_unit, device)) {
    }
    // else if (request_xxx_config(control_unit, device)) {
    // }
    else {
    }

    return device;
}

std::vector<AdbDeviceFinder::Emulator> AdbDeviceFinder::find_emulators() const
{
    LogFunc;

    std::vector<Emulator> result;
    std::unordered_set<std::filesystem::path> seen_adb_paths;

    auto all_processes = list_processes();

    const auto& emu_constant = get_emulator_const_data();

    for (const auto& process : all_processes) {
        auto find_it = std::ranges::find_if(emu_constant, [&process](const auto& pair) -> bool {
            return process.name.find(pair.second.keyword) != std::string::npos;
        });
        if (find_it == emu_constant.cend()) {
            continue;
        }

        auto process_path = get_process_path(process.pid);
        if (!process_path) {
            continue;
        }

        auto adb_path = get_emulator_adb_path(find_it->second, process.pid);

        if (adb_path.empty()) {
            LogWarn << "adb_path is empty or does not exist" << VAR(adb_path);
            continue;
        }

        // Deduplicate by adb_path to distinguish multiple instances or installations
        if (!seen_adb_paths.insert(adb_path).second) {
            continue;
        }

        Emulator emulator {
            .name = find_it->first,
            .process_path = *process_path,
            .adb_path = adb_path,
        };
        result.emplace_back(std::move(emulator));
    }

    LogInfo << VAR(result);

    return result;
}

std::filesystem::path AdbDeviceFinder::get_emulator_adb_path(const EmulatorConstantData& emulator, os_pid pid) const
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

MAA_TOOLKIT_NS_END
